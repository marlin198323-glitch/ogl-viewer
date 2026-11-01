# OGL Firestorm – Anweisungen für Entwicklungsassistenten

Kanonisches Repository:
https://github.com/marlin198323-glitch/ogl-viewer
Arbeitsbranch: ogl-develop

Bei jeder Entwicklungsaufgabe ZUERST lesen:
1. ogl/PROJECT_HANDOFF.md
2. ogl/PROJECT_STATUS.md
3. ogl/GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md
4. Detailplan der aktiven /goal-Phase

Nur diesen Firestorm-Fork bearbeiten, nicht das getrennte
ältere Projekt nexverse-viewer. Auf dem Linux-Mint-PC
jam-MS-7D28 arbeiten; lokaler Pfad:
/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm

Vor Änderungen git status, aktuellen Branch, jüngste
Commits und uncommitted changes prüfen. Keine .env,
Benutzerdaten, Cache-, Login- oder Secret-Dateien
ungefragt überschreiben.

Jede /goal-Phase in 20 überprüfbare Entwicklungsblöcke
gliedern. Nie fehlende Tests als erfolgreich ausgeben.
Code und UI für Linux/Windows/macOS und AMD/Intel-x64/
ARM64 konzipieren. Standard-OpenGL-Fallback für ältere
Hardware erhalten; optionale GPU-/CPU-Funktionen erkennen.

Genau zwei feste Grids:
Stadt NexVerse (OpenSim) und Stadt NexVerse Beta.
Beta-Zugangsdaten nur nach bestätigtem HTTPS und
dokumentiertem Protokoll übertragen.

Verbindlich geplant: Pflicht-Updater mit serverseitiger
Mindestversionsprüfung. Alte nicht mehr zugelassene
Viewer-Versionen dürfen sich nicht anmelden, auch
wenn ein Client eine Updateaufforderung ignoriert.
**Das ist noch nicht implementiert.**

Das originale Firestorm-Design bleibt Standard.
Plugins und Theme-Installationen sicher halten.

Nach Änderungen Tests/Builds dokumentieren, nach
erfolgreicher Prüfung auf origin/ogl-develop pushen
und ogl/PROJECT_STATUS.md sowie die Roadmap aktualisieren.
Keine Aussage über vollständige Windows-/macOS-Releases
allein aus Modul-CI-Tests ableiten.
