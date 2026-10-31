#!/usr/bin/env bash
# Experimental 128x72 Vulkan depth-guided raytracing on real OpenSim data.
# First experimental in-world sunlight compositor: stale camera/geometry
# or mismatched OpenGL depth rejects all output; never used in public mode.
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
export OGL_RTX_SCENE_PROBE=1
export OGL_RTX_CAMERA_PROBE=1
export OGL_RTX_DEPTH_PROBE=1
export OGL_RTX_DENSE_PREVIEW=1
export OGL_RTX_WORLD_COMPOSITE=1
# Diagnostic tint is a separate, explicitly enabled test script only.
# Normal dense preview shows physically motivated darkening.
# Preserve debug tint only when caller explicitly exports 1.
if [ "${OGL_RTX_SHADOW_VISUALIZE:-0}" != "1" ]; then
    unset OGL_RTX_SHADOW_VISUALIZE
fi
unset OGL_RTX_DEBUG_OVERLAY
unset OGL_RTX_DEPTH_GUIDED_PROBE
printf '%s\n' 'OGL RTX experiment: 128x72 live OpenSim Vulkan ray queries.'   'OpenGL stays the public fallback. No fake 8x8 shadow upscaling.'
exec bash "$DIR/run-viewer-test.sh" "$@"
