#!/bin/bash

SCRIPTSRC=`readlink -f "$0" || echo "$0"`
RUN_PATH=`dirname "${SCRIPTSRC}" || echo .`

install_prefix=${RUN_PATH}/..

function install_desktop_entry()
{
    local installation_prefix="$1"
    local desktop_entries_dir="$2"

    local desktop_entry="\
[Desktop Entry]\n\
Name=OGL Viewer\n\
Comment=Client for accessing 3D virtual worlds\n\
Exec=${installation_prefix}/firestorm\n\
Icon=${installation_prefix}/firestorm_icon.png\n\
Terminal=false\n\
Type=Application\n\
Categories=Application;Internet;Network;\n\
StartupNotify=true\n\
X-Desktop-File-Install-Version=3.0\n\
StartupWMClass=ogl-viewer"

    echo " - Installing menu entries in ${desktop_entries_dir}"
    mkdir -vp "${desktop_entries_dir}" || return 1
    printf '%b\n' "$desktop_entry" > "${desktop_entries_dir}/ogl-viewer.desktop" || {
        echo "Failed to install OGL Viewer application menu entry." >&2
        return 1
    }
    # Migrate only the previously generated desktop entry for this exact
    # installation. Never delete a different application or custom launcher.
    local old_entry="${desktop_entries_dir}/firestorm-viewer.desktop"
    if [ -f "$old_entry" ] &&
        grep -Fqx "Exec=${installation_prefix}/firestorm" "$old_entry"; then
        rm -f -- "$old_entry"
    fi
}

if [ "$UID" == "0" ]; then
    # system-wide
    install_desktop_entry "$install_prefix" /usr/local/share/applications
else
    # user-specific
    install_desktop_entry "$install_prefix" "$HOME/.local/share/applications"
fi
