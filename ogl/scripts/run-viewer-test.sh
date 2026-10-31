#!/usr/bin/env bash
# OGL Firestorm baseline test launcher; does not replace the legacy OGL viewer.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
STAGE="$ROOT/build-linux-x86_64/newview/packaged"
STATE="${XDG_STATE_HOME:-$HOME/.local/state}/ogl-firestorm-test"
PROFILE="${XDG_DATA_HOME:-$HOME/.local/share}/ogl-firestorm-test"
mkdir -p "$STATE" "$PROFILE"

if [ ! -x "$STAGE/bin/do-not-directly-run-firestorm-bin" ] || [ ! -f "$STAGE/firestorm" ]; then
  echo "OGL Test: Firestorm-Build oder Paketressourcen fehlen." >&2
  echo "Zuerst bauen: cmake --build build-linux-x86_64 --target copy_l_viewer_manifest --parallel 6" >&2
  exit 2
fi

# Preserve the upstream runtime wrapper, but do not register desktop protocol
# handlers during this local test. Never patch the upstream source wrapper.
SAFE_WRAPPER="$STAGE/ogl-firestorm-test-wrapper"
sed \
  -e '/^test -x \.\/etc\/register_hopprotocol\.sh && \.\/etc\/register_hopprotocol\.sh$/d' \
  -e '/^test -x \.\/etc\/register_secondlifeprotocol\.sh && \.\/etc\/register_secondlifeprotocol\.sh$/d' \
  "$STAGE/firestorm" > "$SAFE_WRAPPER"
chmod 700 "$SAFE_WRAPPER"

# The app's Linux directory code reads the uppercased APP_NAME + _x64 key.
# This keeps all Firestorm-based test settings/cache out of ~/.firestorm_x64.
export OGL_X64_USER_DIR="$PROFILE"
export FIRESTORM_X64_USER_DIR="$PROFILE" # compatibility with the prior test binary
export __GL_THREADED_OPTIMIZATIONS=1

if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
  echo "No graphical session. Start from the Linux Mint desktop." >&2
  exit 3
fi

LOG="$STATE/launch-$(date +%Y%m%d-%H%M%S).log"
printf 'OGL Viewer development test, log: %s\n' "$LOG"
printf 'Profile: %s\n' "$PROFILE"
cd "$STAGE"
exec "$SAFE_WRAPPER" "$@" >>"$LOG" 2>&1
