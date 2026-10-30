#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BASE="$(dirname "$ROOT")"
echo "OGL / Firestorm Linux build prerequisites"
echo "Source: $ROOT"
echo "Branch: $(git -C "$ROOT" branch --show-current)"

missing=()
for cmd in git cmake ninja python3 pkg-config; do
  command -v "$cmd" >/dev/null 2>&1 || missing+=("$cmd")
done
for package in libgl1-mesa-dev libglu1-mesa-dev libpulse-dev libssl-dev libxinerama-dev libxrandr-dev libfontconfig-dev libfreetype-dev; do
  if ! dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q 'install ok installed'; then
    missing+=("$package")
  fi
done

if command -v gcc-11 >/dev/null 2>&1 && command -v g++-11 >/dev/null 2>&1; then
  echo "Compiler: GCC 11 available (recommended by upstream Linux guide)"
elif command -v g++ >/dev/null 2>&1; then
  echo "Compiler: $(g++ -dumpfullversion); upstream recommends GCC 11 (untested fallback)"
else
  missing+=("g++")
fi

if [ -x "$BASE/.venv-firestorm/bin/autobuild" ]; then
  "$BASE/.venv-firestorm/bin/autobuild" --version
else
  echo "Missing local autobuild: $BASE/.venv-firestorm/bin/autobuild"
  missing+=("autobuild")
fi

if [ -f "$BASE/fs-build-variables/variables" ]; then
  echo "Build variables: available"
else
  echo "Build variables: missing"
  missing+=("fs-build-variables")
fi

if (( ${#missing[@]} > 0 )); then
  printf 'Missing prerequisite: %s\n' "${missing[@]}"
  echo "System packages must be installed by the workstation owner; do not paste passwords into chat."
  exit 1
fi
echo "Static prerequisite check passed; this does not certify a working viewer build."
