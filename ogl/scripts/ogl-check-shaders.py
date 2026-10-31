#!/usr/bin/env python3
"""Reject outdated or modified bundled SPIR-V shader assets before build.

The manifest is a deliberately pinned known-good source/SPV pair.
It does NOT replace SPIR-V validation or compilation. Changing a GLSL
source requires rebuilding its SPV and refreshing both manifest digests.
Run without third-party packages; safe in CI and on OpenGL-only hosts.
"""
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = "ogl/modules/rtx/shaders/spv-lock.json"


def verify_pairs(root: Path, manifest: dict) -> list[str]:
    errors = []
    pairs = manifest.get("pairs")
    if manifest.get("schema") != 1 or not isinstance(pairs,list) or len(pairs)!=3:
        return ["shader manifest has unexpected structure"]
    seen = set()
    for pair in pairs:
        if not isinstance(pair,dict):
            errors.append("invalid shader pair")
            continue
        for name, digest_name in (("source","source_sha256"),
                                  ("compiled","compiled_sha256")):
            name_value = pair.get(name)
            expected = pair.get(digest_name)
            if not isinstance(name_value,str) or not isinstance(expected,str):
                errors.append(f"{name}: invalid manifest entry")
                continue
            candidate = Path(name_value)
            if candidate.is_absolute() or ".." in candidate.parts:
                errors.append(f"{name}: forbidden path")
                continue
            if (name_value, name) in seen:
                errors.append(f"{name}: duplicate entry")
                continue
            seen.add((name_value,name))
            full = root / candidate
            if not full.is_file():
                errors.append(f"{name_value}: missing")
                continue
            calculated = hashlib.sha256(full.read_bytes()).hexdigest()
            if calculated != expected:
                errors.append(f"{name_value}: SHA256 mismatch (recompile source "
                              "and update paired shader digests)")
        if pair.get("source","").rsplit("/",1)[-1].removesuffix(".comp") != (
            pair.get("compiled","").rsplit("/",1)[-1].removesuffix(".comp.spv")):
            errors.append("source and SPIR-V asset names differ")
    return errors


def main() -> int:
    try:
        manifest = json.loads((ROOT / MANIFEST).read_text(encoding="utf-8"))
    except (OSError,ValueError) as error:
        print(f"OGL RTX shader manifest unavailable: {error}",file=sys.stderr)
        return 1
    errors = verify_pairs(ROOT,manifest)
    if errors:
        for error in errors:
            print("OGL RTX shader integrity ERROR:",error,file=sys.stderr)
        print("Rebuild changed GLSL files with glslangValidator and spirv-val, "
              "then refresh spv-lock.json.",file=sys.stderr)
        return 1
    print("OGL RTX shader integrity: 3 source/SPIR-V pairs verified")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
