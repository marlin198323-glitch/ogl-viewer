#!/usr/bin/env bash
# Build the Firestorm-based OGL Linux test app without changing the legacy viewer.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BASE="$(dirname "$ROOT")"
VENV="$BASE/.venv-firestorm"
cd "$ROOT"

if [ ! -x "$VENV/bin/autobuild" ]; then
  echo "Missing isolated Autobuild environment: $VENV" >&2
  exit 2
fi

bash ogl/scripts/doctor.sh
export PATH="$VENV/bin:$PATH"
export AUTOBUILD_VARIABLES_FILE="$BASE/fs-build-variables/variables"

# Explicit compiler tooling prevents AUTOBUILD_EXECUTABLE-NOTFOUND errors.
cmake -S indra -B build-linux-x86_64 \
  "-DAUTOBUILD_EXECUTABLE:FILEPATH=$VENV/bin/autobuild" \
  "-DVIEWER_CHANNEL:STRING=OGL Viewer Development" \
  "-DOPENSIM:BOOL=ON" "-DLL_TESTS:BOOL=OFF"
cmake --build build-linux-x86_64 --target firestorm-bin --parallel 6
cmake --build build-linux-x86_64 --target copy_l_viewer_manifest --parallel 5

# To avoid stale login textures on incremental CMake builds, regenerate the
# manifest when this OGL-specific branding source is newer than the copied one.
BRAND_SRC="$ROOT/indra/newview/skins/default/textures/windows/login_ogl_logo.png"
BRAND_DST="$ROOT/build-linux-x86_64/newview/packaged/skins/default/textures/windows/login_ogl_logo.png"
if [ -f "$BRAND_SRC" ] && { [ ! -f "$BRAND_DST" ] || [ "$BRAND_SRC" -nt "$BRAND_DST" ]; }; then
  echo "Regenerating staged resources for changed OGL branding."
  STAMP_DIR="$ROOT/build-linux-x86_64/newview"
  # Remove only the generated CMake copy marker; never delete original assets.
  find "$STAMP_DIR" -maxdepth 1 -type f -name '*.copy_touched' -delete
  cmake --build build-linux-x86_64 --target copy_l_viewer_manifest --parallel 5
fi

bash ogl/scripts/install-test-shortcut.sh
python3 -m unittest discover -s ogl/tests -p 'test_*.py' -v
ctest --test-dir "$BASE/.build-ogl-modules" --output-on-failure
echo "OGL Viewer is built. Launch with:"
echo "  bash "$ROOT/ogl/scripts/run-viewer-test.sh""
if [ "${1:-}" = "--run" ]; then
  exec bash ogl/scripts/run-viewer-test.sh
fi
