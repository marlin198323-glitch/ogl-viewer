#!/usr/bin/env bash
# Install a local-only desktop entry for the OGL Firestorm baseline test.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
LAUNCH="$ROOT/ogl/scripts/run-viewer-test.sh"
ICON="$ROOT/indra/newview/skins/default/textures/windows/login_ogl_logo.png"
APP_DIR="$HOME/.local/share/applications"
DESKTOP_DIR="$HOME/Schreibtisch"
ENTRY="$APP_DIR/ogl-firestorm-test.desktop"

if [ ! -x "$LAUNCH" ]; then
  echo "Launcher missing or not executable: $LAUNCH" >&2
  exit 1
fi

mkdir -p "$APP_DIR"
cat > "$ENTRY" <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=OGL Viewer (Firestorm-Test)
Comment=Isolierter OpenSim-Test des Firestorm-basierten OGL Viewers
Exec=$LAUNCH
Icon=$ICON
Terminal=false
Categories=Game;Graphics;
StartupNotify=true
EOF
chmod 755 "$ENTRY"

if [ -d "$DESKTOP_DIR" ]; then
  cp "$ENTRY" "$DESKTOP_DIR/OGL-Firestorm-Test.desktop"
  chmod 755 "$DESKTOP_DIR/OGL-Firestorm-Test.desktop"
  if command -v gio >/dev/null 2>&1; then
    gio set "$DESKTOP_DIR/OGL-Firestorm-Test.desktop" metadata::trusted true >/dev/null 2>&1 || :
  fi
fi

echo "Menu entry: $ENTRY"
echo "Desktop shortcut: $DESKTOP_DIR/OGL-Firestorm-Test.desktop"
