#!/usr/bin/env python3
"""Local OGL test health summary (no credentials or full log lines printed)."""
from pathlib import Path
import os
import re
import shutil
import subprocess

HOME = Path.home()
ROOT = Path(__file__).resolve().parents[2]
PROFILE = Path(os.environ.get("XDG_DATA_HOME", str(HOME / ".local/share"))) / "ogl-firestorm-test"
LOG = PROFILE / "logs/OGL.log"  # after the fork APP_NAME changes
BINARY = ROOT / "build-linux-x86_64/newview/packaged/bin/do-not-directly-run-firestorm-bin"
CACHE = ROOT / "build-linux-x86_64/CMakeCache.txt"


def disk_size(path: Path) -> str:
    if not path.exists():
        return "not yet created"
    try:
        result = subprocess.run(
            ["du", "-sh", "--", str(path)],
            check=True, capture_output=True, text=True, timeout=8
        )
        return result.stdout.split()[0]
    except (OSError, subprocess.SubprocessError, IndexError):
        return "unavailable"


def report():
    print("OGL Viewer – local Linux Mint test")
    print("Binary:", "present" if BINARY.is_file() else "missing")
    print("Test profile:", PROFILE)
    if CACHE.exists():
        cmake = CACHE.read_text(errors="replace")
        for key in ("VIEWER_CHANNEL", "OPENSIM"):
            value = re.search(rf"^{key}:[^=]+=([^\n]*)", cmake, re.M)
            print(f"{key}:", value.group(1) if value else "unknown")
    if LOG.exists():
        # Numeric counts only: avoid leaking account names, URLs or credentials.
        text = LOG.read_text(errors="replace")
        print("Log file:", LOG)
        print("Error entries:", len(re.findall(r"\b(?:ERROR|FATAL)\b", text)))
        print("Warning entries:", len(re.findall(r"\bWARNING\b", text)))
        print("Login screen ready:", "STATE_LOGIN_WAIT" in text)
        print("Viewer stopped cleanly:", "cleanup : Goodbye!" in text)
        rtx_lines = re.findall(
            r"OGLRTX[^\n]*GPU [^\n]*hardware_ready=(\d+) rendering_active=(\d+)",
            text
        )
        if rtx_lines:
            ready, active = rtx_lines[-1]
            print("RTX Vulkan hardware:", "supported" if ready == "1" else "not supported")
            print("Ray-traced world rendering:", "active" if active == "1" else "not implemented")
        else:
            print("RTX module status: not reported")
    else:
        print("Viewer log: not yet created")
    print("Application data size:", disk_size(PROFILE))
    for name in ("cache", "texturecache", "objectcache", "logs"):
        path = PROFILE / name
        if path.exists():
            print(f"  {name}: {disk_size(path)}")
    if shutil.which("nvidia-smi"):
        try:
            gpu = subprocess.run(
                ["nvidia-smi", "--query-gpu=name,driver_version",
                 "--format=csv,noheader"],
                capture_output=True, text=True, timeout=3
            )
            if gpu.returncode == 0:
                print("GPU:", gpu.stdout.strip().splitlines()[0])
        except (OSError, subprocess.SubprocessError, IndexError):
            pass
    print("Note: disk size and logs do not measure actual texture/mesh cache hit rate.")


if __name__ == "__main__":
    report()
