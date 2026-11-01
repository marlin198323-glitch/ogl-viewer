# OGL branding and login-safety integration

**Scope:** A genuine Firestorm 7.2.5 OpenSim fork with OGL test branding.
The branded Linux Mint build and graphical startup are verified (2026-10-31):
OGL Viewer Development 7.2.5.6; NexVerse HTTPS welcome; OpenGL 4.6.
This is the development branch, not a production-ready OGL release.

## Implemented in source

- Application identity: `APP_NAME = "OGL"`. Display name in the default
  localization: `OGL Viewer`; development channel: `OGL Viewer Development`.
- User profile isolation on Linux:
  `OGL_X64_USER_DIR=~/.local/share/ogl-firestorm-test`.
- Original OpenGenesisLINK icon reused from the existing local
  `nexverse-viewer/assets/branding` directory. The older project is untouched.
  The icon is installed in the modern login panel and Linux icon resources.
- Welcome content: `https://stadt-nexverse.de/service/welcome`.
  This is a web-page background, **not** a substitute for the grid login URI.
- Interactive login UI requires a localized per-process confirmation for
  the pinned Stadt NexVerse HTTP endpoint. The last authentication boundary
  checks consent too; all unexpected HTTP endpoints remain blocked.
- Existing upstream Firestorm/Second Life copyright and third-party
  acknowledgments are retained. The executable may retain upstream internal
  code names and still includes upstream Firestorm features.

## P01 – Login modernization in progress (2026-11-01)

The default OGL login panel now uses a compact 140-unit footer instead of
152 units, with shared OGL colors in the existing `skins/default/colors.xml`.
The standard NexVerse welcome browser remains unchanged and the two fixed
grids retain their existing security guards. OpenGL/Vulkan RT diagnostics
have been removed from the login panel; renderer logging and diagnostics
elsewhere are untouched. The legacy mode selector is hidden in the OGL
login while saved settings remain available internally. Both remembered
username and password controls remain intact and keyboard accessible.
English and German UI overrides are checked by static tests.

Account creation and forgotten-password actions for OGL open the
verified NexVerse homepage, not the upstream Firestorm/Second Life site.
A dedicated recovery URL still needs verification before use. No change
was made to secrets, credentials or the live account system.

A full local Linux viewer build, 130 Python tests and 41 native tests
passed. Virtual Xvfb 1280x720 GUI launches confirm the initial 1024x680
OGL viewer window stays inside the 720p screen and loads the welcome URL.
A 3840x2160 virtual display starts the viewer centered. OGL's fixed grid
dropdown shows exactly Stadt NexVerse and Stadt NexVerse Beta with no
free-form grid address entry. The StarLight skin also displayed the OGL
login in an isolated 35-second GUI test; the current skin-XUI loader
requires complete overlays, so the two existing StarLight XML files
are currently kept aligned with the canonical OGL panel.
The test was time-limited and did not validate real-world OpenSim login,
high-DPI rendering, a clean application shutdown or platform installers.
OGL's EN/DE startup antivirus guidance no longer links to Firestorm's wiki
or encourages unconditional antivirus exceptions. P01 remains open at block 19/20. Old mode and grid-builder controls were removed from 12 existing localized/alternate OGL login XUI files; the non-OGL legacy login remains for compatibility. All three tested Linux skins showed the OGL welcome/login without missing dummy widgets. Updated Linux Xvfb screenshots confirm complete login fields at 1280x720 (UI request 150%, capped), 1920x1080 (UI 150%, 1536x1020 window) and 3840x2160 (UI 200%, 2048x1360 window). Windows and macOS remain untested.

During P01 block 19, the Linux GUI also confirmed the fixed welcome
URL cannot be replaced by a saved ForceLoginURL override. The OGL
pre-login Ctrl+Q closes normally (exit code 0) without a redundant
legacy confirmation dialog. The isolated FSRememberUsername setting
survived a viewer restart; this does not prove successful account
login or stored-password restoration. Real main-grid and full
Windows/macOS viewer tests remain pending.

P01 block 19 also disables the OGL viewer's unrelated Phoenix/Firestorm
configuration, agent, script and client-tag downloads while retaining
startup completion flags and the non-OGL compatibility code. An isolated
Linux Xvfb run verified that no phoenixviewer.com fetches occurred, the
fixed NexVerse welcome loaded, the embedded user agent contained
`OGL skin` rather than `firestorm skin`, and Ctrl+Q exited cleanly.
The local gate was 135 Python tests and 41 native CTests. A separate
20/20 security/build verification batch succeeded, but P01 remains
18/20 pending a real authenticated grid login and full platform builds.

P01 block 19 desktop rebranding: an installed OGL desktop entry now
advertises `Name=OGL Viewer` and `StartupWMClass=ogl-viewer`.
Existing installations are migrated only if the previous Firestorm
launcher points to the same packaged binary. The SDL/X11 runtime
directly sets its actual XClassHint (an SDL environment hint alone
failed in a real GUI smoke); Xvfb now reports WM_CLASS
`("ogl-viewer", "ogl-viewer")`. Three GUI smoke tests at 720p,
1080p/150% and 4K/200% loaded the welcome page and exited
cleanly. OGL configuration/display errors no longer send users
to Firestorm support websites. The latest local gate is
136 Python tests plus 41 native CTests. Real Wayland,
full Windows/macOS viewer builds and authenticated grid login
remain unverified; P01 stays 18/20.

## HTTP login compatibility

The fixed Stadt NexVerse login endpoint is `http://hg.stadt-nexverse.de:80/`.
The user explicitly requested HTTP compatibility. The OGL login panel
asks for a per-process confirmation before connecting to **this exact**
HTTP endpoint, and the authentication boundary checks the same approval.
Unattended HTTP login without that consent and all other HTTP grids stay
blocked. HTTP traffic remains unencrypted; use a separate test account
when possible. An optional HTTPS server upgrade can be performed later.

## Local Linux test

```sh
cd "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm"
bash ogl/scripts/build-test.sh
bash ogl/scripts/run-viewer-test.sh
```

Rebuilding recompiles C++ only when necessary, then stages resources and
installs a local desktop shortcut. To build and open in one command, use:

```sh
bash ogl/scripts/build-test.sh --run
```

## Verification checklist

1. CMake reports `Compiling with OpenSim support` and
   `VIEWER_CHANNEL:STRING=OGL Viewer Development`.
2. C++ executable links, viewer manifest copies skins and Linux libraries.
3. Desktop window title uses OGL branding; the login panel displays the
   original OpenGenesisLINK logo and NexVerse welcome.
4. Verify English and German layouts, small and large screen widths,
   all fonts, CEF rendering and closing behavior.
5. Confirm separate settings/cache/log paths.
6. Verify that HTTP login prompts for explicit consent and that cancelling
   does not authenticate; only the exact Stadt NexVerse URI is allowed.
7. Auto-login without an in-process confirmation must remain blocked;
   unsupported HTTP grids must remain rejected.
8. Only after login: check sky, sun, textures, mesh, avatar appearance,
   camera, animation, teleports, and asset-cache measurements.

The OGL module ABI proof of concept is still standalone; a production
module loader is not wired into Firestorm. Native modules are not
sandboxed. Do not load untrusted modules.
