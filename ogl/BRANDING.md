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
