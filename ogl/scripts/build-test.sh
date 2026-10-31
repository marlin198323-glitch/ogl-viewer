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
# A GLSL-only edit must never ship an older precompiled compute shader.
# The stale depth-guided SPIR-V masked GPU depth mismatches in the live world.
python3 ogl/scripts/ogl-check-shaders.py
export PATH="$VENV/bin:$PATH"
export AUTOBUILD_VARIABLES_FILE="$BASE/fs-build-variables/variables"

# Optional Vulkan SDK headers: use an explicit path, a development copy, or
# allow CMake to locate the system Vulkan-Headers package. GPU vendor is
# never selected here; no Vulkan RT SDK means normal OpenGL-only build.
CMAKE_RTX_ARGS=()
if [ -n "${OGL_VULKAN_HEADER_DIR:-}" ] && [ -d "$OGL_VULKAN_HEADER_DIR/vulkan" ]; then
  CMAKE_RTX_ARGS+=("-DOGL_VULKAN_HEADER_DIR:PATH=$OGL_VULKAN_HEADER_DIR")
elif [ -d "$BASE/.vulkan-sdk-local/headers/include/vulkan" ]; then
  CMAKE_RTX_ARGS+=("-DOGL_VULKAN_HEADER_DIR:PATH=$BASE/.vulkan-sdk-local/headers/include")
fi

# Explicit compiler tooling prevents AUTOBUILD_EXECUTABLE-NOTFOUND errors.
cmake -S indra -B build-linux-x86_64 \
  "-DAUTOBUILD_EXECUTABLE:FILEPATH=$VENV/bin/autobuild" \
  "-DVIEWER_CHANNEL:STRING=OGL Viewer Development" \
  "-DOPENSIM:BOOL=ON" "-DLL_TESTS:BOOL=OFF" "${CMAKE_RTX_ARGS[@]}"
cmake --build build-linux-x86_64 --target firestorm-bin --parallel 6
cmake --build build-linux-x86_64 --target copy_l_viewer_manifest --parallel 5

# Additional standalone capability/ABI tests for development: the optional
# vendor-neutral RT module itself is now built and staged by Firestorm CMake.
MODULE_BUILD="$BASE/.build-ogl-modules"
VULKAN_HEADERS="$BASE/.vulkan-sdk-local/headers/include"
cmake -S ogl/modules -B "$MODULE_BUILD" -G Ninja \
  "-DOGL_VULKAN_HEADER_DIR=$VULKAN_HEADERS"
cmake --build "$MODULE_BUILD" --parallel 4
if [ -f "$MODULE_BUILD/ogl_rtx.so" ]; then
  install -m 755 "$MODULE_BUILD/ogl_rtx.so" \
    "$ROOT/build-linux-x86_64/newview/packaged/lib/ogl_rtx.so"
fi

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
