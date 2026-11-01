# OGL Firestorm Fork – Informationen für einen neuen Chat

Aktuelles Projekt: https://github.com/marlin198323-glitch/ogl-viewer
Branch: ogl-develop
Git-Remote: https://github.com/marlin198323-glitch/ogl-viewer.git

Linux-Mint-PC: jam-MS-7D28
Desktop-Commander-ID: 3a1e9ba9-ea8a-4650-b8f7-84f8f653f20d
Projektpfad: /home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm

Teststart: bash ogl/scripts/run-viewer-test.sh
Build und Tests: bash ogl/scripts/build-test.sh

## Verbindliche Projektquellen

- **Hauptprojektplan mit sämtlichen Arbeitsvorgaben:**
  [GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md](GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md)
- **Aktueller Umsetzungsstand / nächste offene Schritte:**
  [PROJECT_STATUS.md](PROJECT_STATUS.md)
- **Pflichtupdate-Anforderung und 20-Blöcke-Plan:**
  [GOAL_MANDATORY_VIEWER_UPDATER.md](GOAL_MANDATORY_VIEWER_UPDATER.md)
- **Hinweise für Coding-Assistenten im Repository:** ../AGENTS.md

Lese diese Dateien zu Beginn jeder neuen Projektunterhaltung;
bei jedem Entwicklungsschritt die Statusdatei mit tatsächlichen
Test-/Abnahmeergebnissen aktualisieren. Das vorhandene GitHub-Repository
ist die verbindliche Projektquelle. Die Dateien werden in neuen
Chats nicht ohne Repositoryzugriff oder ausdrücklichen Hinweis
automatisch geladen.

## Arbeitsregeln

- Nur ogl-viewer auf Branch ogl-develop bearbeiten, nicht das ältere
  Repo nexverse-viewer.
- Zuerst git status und letzten Commit prüfen.
- /goal phasenweise, jeweils 20 Entwicklungsblöcke.
- Ergebnisse testen und direkt auf ogl-develop veröffentlichen.
- Linux, Windows und macOS, AMD/Intel x86-64 und ARM64 bedenken.
- CPU-/GPU-Erkennung und OpenGL-Standardpfad für ältere Hardware.
- Originales Firestorm-Design bleibt der Standard.
- Feste Grid-Auswahl: Stadt NexVerse und Stadt NexVerse Beta.
- Beta-Anmeldung bleibt bis zur bestätigten HTTPS-/Protokoll-
  Unterstützung gesperrt.
- Verbindlich geplant: ein Pflicht-Updater, der vor der
  Anmeldung prüft, ob die Server-Mindestversion erfüllt ist.
  Veraltete Versionen erhalten keine Serveranmeldung, wenn
  das neue Update noch nicht installiert wurde.

## Aktuelle Roadmaps

- ogl/GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md
- ogl/PROJECT_STATUS.md
- ogl/GOAL_MANDATORY_VIEWER_UPDATER.md
- ogl/GOAL_THEME_PLUGINS.md
- ogl/PLUGIN_SYSTEM_CROSS_PLATFORM_ARCHITECTURE.md
- ogl/NEXVERSE_GRID.md

## Kompakter Starttext für einen neuen Chat

Bitte setze mein OGL Firestorm Fork Projekt fort.
GitHub: https://github.com/marlin198323-glitch/ogl-viewer
Branch: ogl-develop
Nutze den Linux-Mint-Commander jam-MS-7D28 und
/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm.
Lies zuerst AGENTS.md, ogl/PROJECT_HANDOFF.md,
ogl/PROJECT_STATUS.md und die /goal-Roadmaps.
Arbeite immer 20 Entwicklungsblöcke pro Phase ab.
Linux, Windows und macOS sowie AMD/Intel/ARM64 beachten.
Beide feste NexVerse-Grids erhalten. Ein verpflichtender
Viewer-Updater muss vom Viewer und Server unterstützt werden;
alte, nicht mehr zugelassene Versionen dürfen sich nicht
anmelden. Änderungen testen, direkt auf GitHub pushen,
den dokumentierten Projektstatus aktualisieren und
die nächsten offenen Schritte nennen.
