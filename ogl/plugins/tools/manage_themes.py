#!/usr/bin/env python3
"""OGL portable theme installer (user-data only, no native code).

This tool installs and selects declarative packages; the user must press
Activate in the Viewer Preferences > OGL Designs tab to apply their colors.
No Python interpreter or code plugin is executed inside Firestorm.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import zipfile

from validate_theme import UnsafeTheme, validate_theme, ID

ACTIVATION="active-theme.json"

def default_data_dir()->Path:
    """Never write to executable/source folders or an OS installation area."""
    home=Path.home()
    if sys.platform=="win32":
        base=os.environ.get("APPDATA")
        return (Path(base) if base else home/"AppData"/"Roaming")/"OGL Viewer"
    if sys.platform=="darwin":
        return home/"Library"/"Application Support"/"OGL Viewer"
    xdg=os.environ.get("XDG_DATA_HOME")
    return (Path(xdg) if xdg and Path(xdg).is_absolute()
            else home/".local"/"share")/"ogl-viewer"

def safe_id(value:str)->str:
    if not isinstance(value,str) or len(value)>96 or not ID.fullmatch(value):
        raise UnsafeTheme("Invalid theme identifier")
    return value

def safe_themes(root:Path)->Path:
    # In particular, never run recursive deletion inside a symlinked themes dir.
    if root.is_symlink():
        raise UnsafeTheme("Theme data directory must not be a symlink")
    root.mkdir(parents=True,exist_ok=True)
    path=root/"themes"
    if path.is_symlink():
        raise UnsafeTheme("Themes directory must not be a symlink")
    path.mkdir(exist_ok=True)
    return path

def active_id(root:Path)->str|None:
    state=root/ACTIVATION
    if state.is_symlink():
        raise UnsafeTheme("Active theme state must not be a symlink")
    if not state.exists(): return None
    try:
        obj=json.loads(state.read_text(encoding="utf-8"))
    except (ValueError,OSError) as exc:
        raise UnsafeTheme("Invalid active theme state") from exc
    if not isinstance(obj,dict) or set(obj)!={"id"}:
        raise UnsafeTheme("Invalid active theme state format")
    return None if obj["id"] is None else safe_id(obj["id"])

def write_active(root:Path,identifier:str|None)->None:
    if identifier is not None: safe_id(identifier)
    if (root/ACTIVATION).is_symlink():
        raise UnsafeTheme("Active theme state must not be a symlink")
    fd,name=tempfile.mkstemp(prefix=".active-",suffix=".json",dir=root)
    try:
        with os.fdopen(fd,"w",encoding="utf-8") as file:
            json.dump({"id":identifier},file,ensure_ascii=True)
            file.write("\n")
            file.flush()
            os.fsync(file.fileno())
        os.replace(name,root/ACTIVATION)
    finally:
        if os.path.exists(name): os.unlink(name)

def check_target(parent:Path,identifier:str)->Path:
    path=parent/safe_id(identifier)
    if path.is_symlink():
        raise UnsafeTheme("Refusing symlinked theme directory")
    return path

def install(root:Path,package:Path)->str:
    doc=validate_theme(package)
    identifier=safe_id(doc["id"])
    parent=safe_themes(root)
    final=check_target(parent,identifier)
    if final.exists(): raise UnsafeTheme("Theme already installed; remove it first")
    staging=Path(tempfile.mkdtemp(prefix=".theme-",dir=parent))
    try:
        with zipfile.ZipFile(package) as archive:
            for info in archive.infolist():
                dest=staging.joinpath(*info.filename.split("/"))
                dest.parent.mkdir(parents=True,exist_ok=True)
                # Small, prevalidated files only; never trust ZipFile.extract().
                data=archive.read(info)
                if len(data)!=info.file_size:
                    raise UnsafeTheme("Corrupt theme archive content")
                if info.filename!="theme.json":
                    check_image(data,info.filename)
                with dest.open("xb") as file:
                    file.write(data)
        if final.exists(): raise UnsafeTheme("Theme already installed")
        os.replace(staging,final)
    finally:
        if staging.exists(): shutil.rmtree(staging)
    return identifier

def check_image(data:bytes,name:str)->None:
    # Structural pre-screening, not a substitute for decoder hardening.
    if name.lower().endswith(".png"):
        if (len(data)<24 or data[:8]!=b"\x89PNG\r\n\x1a\n" or
            data[12:16]!=b"IHDR"):
            raise UnsafeTheme("Invalid PNG image")
        width=int.from_bytes(data[16:20],"big")
        height=int.from_bytes(data[20:24],"big")
    elif name.lower().endswith(".webp"):
        if (len(data)<16 or data[:4]!=b"RIFF" or
            data[8:12]!=b"WEBP" or int.from_bytes(data[4:8],"little")+8!=len(data)):
            raise UnsafeTheme("Invalid WebP image")
        # Decoder performs stricter validation after activation.
        return
    else:
        raise UnsafeTheme("Unsupported image asset")
    if not (1<=width<=4096 and 1<=height<=4096):
        raise UnsafeTheme("Image dimensions exceed safe limit")

def list_installed(root:Path)->list[dict]:
    parent=safe_themes(root)
    result=[]
    for folder in parent.iterdir():
        if not folder.is_dir() or folder.is_symlink() or folder.name.startswith("."):
            continue
        if not ID.fullmatch(folder.name): continue
        manifest=folder/"theme.json"
        if not manifest.is_file() or manifest.is_symlink() or manifest.stat().st_size>16384:
            continue
        try:
            from validate_theme import validate_manifest
            doc=validate_manifest(json.loads(manifest.read_text(encoding="utf-8")))
        except (UnsafeTheme,ValueError,OSError):
            continue
        if doc["id"]==folder.name:
            result.append({"id":doc["id"],"name":doc["name"],"version":doc["version"],
                           "active":active_id(root)==doc["id"]})
    return sorted(result,key=lambda x:x["id"])

def activate(root:Path,identifier:str)->None:
    parent=safe_themes(root)
    folder=check_target(parent,identifier)
    if not folder.is_dir() or folder.is_symlink():
        raise UnsafeTheme("Theme not installed")
    if not any(record["id"]==identifier for record in list_installed(root)):
        raise UnsafeTheme("Installed theme manifest invalid")
    write_active(root,identifier)

def remove(root:Path,identifier:str)->None:
    parent=safe_themes(root)
    path=check_target(parent,identifier)
    if not path.is_dir(): raise UnsafeTheme("Theme not installed")
    if active_id(root)==identifier:
        write_active(root,None)
    shutil.rmtree(path)

def main(argv:list[str])->int:
    parser=argparse.ArgumentParser(description="Manage portable OGL color themes; activate colors in Viewer preferences")
    parser.add_argument("--data-dir",type=Path,default=default_data_dir())
    sub=parser.add_subparsers(dest="command",required=True)
    ins=sub.add_parser("install");ins.add_argument("package",type=Path)
    sub.add_parser("list")
    active=sub.add_parser("activate");active.add_argument("id")
    sub.add_parser("reset")
    rm=sub.add_parser("remove");rm.add_argument("id")
    args=parser.parse_args(argv)
    try:
        if args.command=="install":
            print("Installed:",install(args.data_dir,args.package))
        elif args.command=="list":
            print(json.dumps(list_installed(args.data_dir),indent=2))
        elif args.command=="activate":
            activate(args.data_dir,args.id);print("Selected:",args.id)
        elif args.command=="reset":
            safe_themes(args.data_dir)
            write_active(args.data_dir,None);print("Selected: built-in Firestorm")
        elif args.command=="remove":
            remove(args.data_dir,args.id);print("Removed:",args.id)
    except (UnsafeTheme, OSError) as exc:
        print("Theme operation refused:",exc,file=sys.stderr)
        return 1
    print("Note: CLI changes package metadata only. Open Viewer Preferences > OGL Designs and click Activate design (or Restore original) to change rendered colors.")
    return 0

if __name__=="__main__":
    sys.exit(main(sys.argv[1:]))
