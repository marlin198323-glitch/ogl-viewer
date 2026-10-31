#!/usr/bin/env bash
# Developer-only RTX result preview inside the real OGL Viewer.
# Displays a labeled 8x8 diagnostic panel, NOT raytraced world shadows.
set -euo pipefail
dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export OGL_RTX_SCENE_PROBE=1
export OGL_RTX_CAMERA_PROBE=1
export OGL_RTX_DEPTH_PROBE=1
export OGL_RTX_DEPTH_GUIDED_PROBE=1
export OGL_RTX_DEBUG_OVERLAY=1
# Preserve the normal safety throttle. Avoid accelerated GPU probes in the UI.
unset OGL_RTX_DEV_INTERVAL_MS
printf '%s\n' 'OGL RTX DEBUG 8x8 — keine Schatten in der eigentlichen 3D-Welt.' \
    'Nach dem Stadt-NexVerse-Login erscheint die Vorschau nur bei gültigen GPU-Daten.' \
    'Fehlt Vulkan Ray Query, bleibt der normale OpenGL Viewer aktiv.'
exec bash "$dir/run-viewer-test.sh" "$@"
