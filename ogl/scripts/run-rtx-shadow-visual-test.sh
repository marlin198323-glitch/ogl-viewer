#!/usr/bin/env bash
# Developer-only visual comparison: highlight ONLY Vulkan sun-shadowed
# surface pixels that pass the LIVE GL camera/depth receiver checks.
# The normal dense script has no debug tint; OpenGL-only default untouched.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export OGL_RTX_SHADOW_VISUALIZE=1
printf '%s\n' 'OGL RTX visual comparison: red tint indicates validated ray-traced' \
 'sun shadow receivers. No tint means no accepted current GL pixels.'
exec bash "$SCRIPT_DIR/run-rtx-dense-preview-test.sh" "$@"
