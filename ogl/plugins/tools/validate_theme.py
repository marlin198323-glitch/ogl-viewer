#!/usr/bin/env python3
"""Validate portable .ogltheme archives. VALIDATION ONLY, no installation.

No scripts, libraries, XML, UI controls, links, remote resources, or
overrides of security-sensitive login UI are accepted. This is a foundation
for a future, opt-in GUI theme installer on all release platforms.
"""
from __future__ import annotations

import json
import re
import stat
import sys
import zipfile
from pathlib import Path, PurePosixPath

MAX_FILES=64
MAX_FILE_SIZE=2 * 1024 * 1024
MAX_TOTAL_SIZE=8 * 1024 * 1024
MAX_MANIFEST=16 * 1024
MAX_COMPRESSION_RATIO=200
TOKEN_NAMES=frozenset({
    "window.background", "window.text", "window.border",
    "accent", "accent.text", "chat.background", "chat.text",
    "chat.header", "chat.timestamp", "button.background",
    "button.text", "button.border"
})
OS_NAMES=frozenset(("linux","windows","macos"))
ID=re.compile(r"[a-z][a-z0-9]*(?:[.-][a-z0-9]+)*\Z",re.ASCII)
VERSION=re.compile(r"0|[1-9][0-9]*")
SEMVER=re.compile(r"(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\Z",re.ASCII)
COLOR=re.compile(r"#[0-9a-fA-F]{6}(?:[0-9a-fA-F]{2})?\Z",re.ASCII)
ASSET=re.compile(r"assets/[a-zA-Z0-9_./-]+\Z",re.ASCII)

class UnsafeTheme(ValueError):
    pass

def validate_manifest(doc: object)->dict:
    if not isinstance(doc,dict):
        raise UnsafeTheme("Theme manifest must be an object")
    required={"schema_version","id","name","version","type",
              "platforms","colors"}
    if set(doc)!=required:
        raise UnsafeTheme("Missing or forbidden manifest keys")
    if type(doc["schema_version"]) is not int or doc["schema_version"]!=1:
        raise UnsafeTheme("Unsupported manifest schema")
    if doc["type"]!="theme":
        raise UnsafeTheme("Native extensions are not themes")
    if not isinstance(doc["id"],str) or len(doc["id"])>96 or not ID.fullmatch(doc["id"]):
        raise UnsafeTheme("Invalid theme id")
    if not isinstance(doc["name"],str) or not 1<=len(doc["name"])<=70:
        raise UnsafeTheme("Invalid theme name")
    if any(ord(c)<32 or ord(c)==127 for c in doc["name"]):
        raise UnsafeTheme("Control characters in theme name")
    if (not isinstance(doc["version"],str) or len(doc["version"])>32 or
        not SEMVER.fullmatch(doc["version"])):
        raise UnsafeTheme("Invalid theme semantic version")
    if (not isinstance(doc["platforms"],list) or
        len(doc["platforms"])!=3 or
        any(type(p) is not str for p in doc["platforms"]) or
        set(doc["platforms"])!=OS_NAMES):
        raise UnsafeTheme("A portable theme must target Linux, Windows and macOS")
    colors=doc["colors"]
    if (not isinstance(colors,dict) or not colors or
        len(colors)>len(TOKEN_NAMES)):
        raise UnsafeTheme("Invalid color map")
    for key,value in colors.items():
        if key not in TOKEN_NAMES or not isinstance(value,str) or not COLOR.fullmatch(value):
            raise UnsafeTheme("Unknown color token or invalid hexadecimal color")
    return doc

def validate_theme(path: Path)->dict:
    try:
        # Reject appended payloads, malformed oversized ZIPs, and accidental
        # attempts to process arbitrary huge files before opening the archive.
        if path.stat().st_size > 16 * 1024 * 1024:
            raise UnsafeTheme("Compressed theme archive is too large")
        with zipfile.ZipFile(path) as archive:
            infos=archive.infolist()
            if not infos or len(infos)>MAX_FILES:
                raise UnsafeTheme("Too many theme entries")
            total=0
            names=set()
            for info in infos:
                # Python zipfile silently converts raw backslashes to '/' on
                # Windows when ZipInfo is constructed while READING a ZIP.
                # Validate the original ZIP name, not only the normalized
                # .filename; otherwise a hostile archive can bypass checks.
                name=info.orig_filename
                if name!=info.filename:
                    raise UnsafeTheme("Platform-dependent ZIP path normalization forbidden")
                # Never interpret Windows backslashes as paths, or allow
                # ZIP absolute/traversal, drive, encoded or hidden paths.
                part=PurePosixPath(name)
                if (not name or not name.isascii() or "\\" in name or ":" in name or
                    name.startswith("/") or name.endswith("/") or
                    any(p in ("","..",".") or p.startswith(".") for p in name.split("/")) or
                    part.as_posix()!=name):
                    raise UnsafeTheme("Unsafe archive path")
                folded=name.casefold()
                if folded in names:
                    raise UnsafeTheme("Duplicate case-insensitive archive path")
                names.add(folded)
                # Reject all non-regular Unix file types, not just symlinks
                # (including FIFOs, block devices, sockets and directories).
                kind=stat.S_IFMT(info.external_attr >> 16)
                if (kind not in (0, stat.S_IFREG) or info.flag_bits & 1):
                    raise UnsafeTheme("Non-regular, symlinked or encrypted theme entries forbidden")
                if info.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED):
                    raise UnsafeTheme("Unsupported ZIP compression method")
                if info.file_size>MAX_FILE_SIZE or info.file_size<0:
                    raise UnsafeTheme("Oversized theme entry")
                if info.file_size and (info.file_size/max(info.compress_size,1))>MAX_COMPRESSION_RATIO:
                    raise UnsafeTheme("Suspicious compression ratio")
                total+=info.file_size
                if total>MAX_TOTAL_SIZE:
                    raise UnsafeTheme("Oversized theme archive")
                if name!="theme.json" and (not ASSET.fullmatch(name) or
                    not name.lower().endswith((".png",".webp"))):
                    raise UnsafeTheme("Only PNG/WebP image assets are permitted")
            if not any(info.filename=="theme.json" for info in infos):
                raise UnsafeTheme("Missing exact theme.json")
            info=archive.getinfo("theme.json")
            if info.file_size>MAX_MANIFEST:
                raise UnsafeTheme("Oversized theme manifest")
            try:
                # A duplicate JSON property is ambiguous across runtimes and
                # must not be interpreted differently by the native viewer.
                def no_duplicate_keys(pairs):
                    result={}
                    for key,value in pairs:
                        if key in result:
                            raise UnsafeTheme("Duplicate theme manifest property")
                        result[key]=value
                    return result
                doc=json.loads(archive.read(info).decode("utf-8"),
                               object_pairs_hook=no_duplicate_keys)
            except (UnicodeError,ValueError) as e:
                raise UnsafeTheme("Invalid UTF-8 JSON manifest") from e
            # A future installer must independently verify PNG/WebP
            # decoding and signatures before writing any file to disk.
            return validate_manifest(doc)
    except (zipfile.BadZipFile, zipfile.LargeZipFile, OSError, RuntimeError, EOFError) as e:
        raise UnsafeTheme("Unreadable theme archive") from e

def main(argv:list[str])->int:
    if len(argv)!=2:
        print("Usage: python3 validate_theme.py PATH.ogltheme",file=sys.stderr)
        return 2
    try:
        doc=validate_theme(Path(argv[1]))
    except UnsafeTheme as e:
        print("REJECTED:",e,file=sys.stderr)
        return 1
    print("VALID portable theme metadata:",doc["id"],doc["version"])
    print("Validation only: NOT installed or activated in the viewer.")
    return 0

if __name__=="__main__":
    sys.exit(main(sys.argv))
