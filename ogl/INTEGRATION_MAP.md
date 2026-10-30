# Verified Firestorm integration map – OGL

All listed paths were confirmed in the checked-out Firestorm_7.2.5 tree.
This is a starting map, **not** a completed integration or a claim of
full feature compatibility.

| Area | Existing source | OGL work |
|---|---|---|
| Linux build / packaging | `indra/newview/CMakeLists.txt` | Add OGL core compile target only after upstream build passes |
| Viewer lifecycle | `indra/newview/llappviewer.cpp` | Controlled plugin manager lifetime (startup/shutdown) |
| Login panel | `indra/newview/llpanellogin.cpp` | Branding, safe remember-login settings, OpenGenesisLINK grid |
| Login/runtime settings | `indra/newview/app_settings/settings.xml` | Isolated OGL settings and defaults |
| Skins/theme | `indra/newview/skins/default/settings.xml` | Modern OGL theme without harming inherited Firestorm controls |
| Avatar wearables | `indra/newview/llagentwearables.cpp` | Real OpenSim appearance/COF behavior validation |
| Avatar renderer | `indra/newview/llvoavatar.cpp` | Skin/clothing/attachments/animesh QA |
| Texture fetch | `indra/newview/lltexturefetch.cpp` | Cache and prioritization measurements before tuning |
| Texture lists | `indra/newview/llviewertexturelist.cpp` | Resident assets and duplicate-download metrics |
| Sky | `indra/newview/llvosky.cpp` | Sun, environment and region windlight/PBR validation |
| Shaders | `indra/newview/llviewershadermgr.cpp` | Graphics quality and fallback matrix |
| Regions | `indra/newview/llviewerregion.cpp` | CAPS, simulator reconnect, teleport validation |
| Preferences | `indra/newview/llfloaterpreference.cpp` | Cache sizing, security, module settings UI |
| Stats | `indra/newview/llviewerstats.cpp` | FPS, first-frame, network, cache hit-rate instrumentation |

## Module architecture gates

1. Standalone versioned C ABI handshake and example module (passed).
2. Thread-safe trusted-module registry and predictable shutdown (not started).
3. Restrict the initially exposed host API (read-only diagnostics first).
4. Platform loaders and symbol-export versioning (Linux probe only).
5. Trust policy, signing, provenance, permissions, and failure isolation.
6. Integrate a controlled module loader behind feature flags only after
   a reproducible baseline viewer build and grid session.
7. Evaluate process isolation for untrusted content; in-process native
   extensions are **not** a sandbox.

## Validation requirements

- Public `http://hg.stadt-nexverse.de/get_grid_info` returned HTTP 200 and
  advertised login `http://hg.stadt-nexverse.de:80/`. HTTPS :443 was not
  reachable. Require a secure login transport before live-account testing.
- Never hard-code user credentials or asset tokens in the source.
- Preserve upstream license obligations, notices and compatible third-party
  media/library licenses.
- Keep the older separate native NexVerse viewer unchanged until the fork
  meets the acceptance gate.
