#!/usr/bin/env python3
"""Switch OGL's pinned Stadt NexVerse login URI only after verified public TLS.

Read-only by default. No login credentials are used, and the remote server is
never changed. Use --activate explicitly to update the local source header.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import sys
import urllib.error
import urllib.request
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
HEADER = ROOT / "indra/newview/oglnexversegrid.h"
GRIDINFO_URL = "https://hg.stadt-nexverse.de/get_grid_info"
SECURE_LOGIN = "https://hg.stadt-nexverse.de/"
OLD_DECLARATION = 'kLoginURI = "http://hg.stadt-nexverse.de:80/";'
NEW_DECLARATION = 'kLoginURI = "https://hg.stadt-nexverse.de/";'


def verify_grid() -> str:
    # urlopen validates the public certificate/hostname using system trust.
    request = urllib.request.Request(GRIDINFO_URL,
                                     headers={"User-Agent": "OGL-Grid-HTTPS-Check/1"})
    with urllib.request.urlopen(request, timeout=12) as response:
        if response.status != 200:
            raise ValueError(f"GridInfo status {response.status}; expected 200")
        if response.geturl() != GRIDINFO_URL:
            raise ValueError("GridInfo redirected to a different address")
        data = response.read(65537)
    if len(data) > 65536:
        raise ValueError("GridInfo XML unexpectedly large")
    root = ET.fromstring(data)
    login = (root.findtext("login") or "").strip()
    if login != SECURE_LOGIN:
        raise ValueError(
            f"GridInfo publishes {login!r}, not the verified secure login URI")
    if (root.findtext("gridname") or "").strip() != "Stadt NexVerse":
        raise ValueError("Grid name mismatch")
    return login


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--check", action="store_true", help="verify only")
    group.add_argument("--activate", action="store_true",
                       help="update local source only after checks pass")
    args = parser.parse_args()

    try:
        login = verify_grid()
        print(f"PASS: TLS validated; GridInfo advertises {login}")
        source = HEADER.read_text()
        if NEW_DECLARATION in source and OLD_DECLARATION not in source:
            print("OGL header already uses verified HTTPS.")
            return 0
        if source.count(OLD_DECLARATION) != 1:
            raise ValueError("OGL header has unexpected login configuration")
        if args.check:
            print("Ready to activate; nothing changed.")
            return 0
        HEADER.write_text(source.replace(OLD_DECLARATION, NEW_DECLARATION))
        print("Updated local OGL kLoginURI to verified HTTPS.")
        print("Next: rebuild OGL, stage resources, run tests, then login with")
        print("a controlled test account. Git changes remain uncommitted.")
        return 0
    except (OSError, urllib.error.URLError, ET.ParseError, ValueError) as exc:
        print(f"BLOCKED: {exc}", file=sys.stderr)
        print("No OGL source was changed.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
