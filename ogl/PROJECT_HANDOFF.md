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

## Automatischer /goal-Phasenlauf

**Maßgeblich sind P01–P13 im bestehenden Hauptprojektplan.**
Nicht mehr frei zwischen den alten Fachkapiteln A–I
wechseln. Aktueller Einstieg: **P01, Block 19/20**
(Blöcke 1–18 abgeschlossen;
`b2520078`: Augenknöpfe
für nicht entschlüsselbare
gespeicherte Passwort-
Platzhalter gesperrt,
manuell eingegebene
Passwörter weiter
anzeigbar, nach Leeren
wieder maskiert.
20/20 technische Checks,
220 Python-/41 native
Tests, DE-Xvfb-GUI und
EN-Xvfb-Screenshot grün;
Hauptgrid-Gridinfo
bestätigt, Beta-TLS-
Zertifikat unpassend.
Beta-GUI-Klicktest nicht
abgeschlossen. P02 0/20;
autorisierter Hauptgrid-
Login, Passwortpersistenz,
In-World-Shutdown und
vollständige Windows-/
macOS-Viewer-GUI-/Build-
Abnahmen noch offen).
Den letzten tatsächlich erledigten Block und Commit immer
in [PROJECT_STATUS.md](PROJECT_STATUS.md) fortschreiben.

Bei `/goal` die aktive Phase in 20 Blöcken durchziehen,
betroffene Tests und Builds ausführen, nach erfolgreicher
technischer Prüfung auf `ogl-develop` pushen.
Unmittelbar mit `/goal Pxx abgeschlossen | 20/20 |
Tests ... | GitHub ...` informieren und ohne Rückfrage
die Folgephase während der aktiven Unterhaltung bearbeiten,
sofern keine echten Blocker entgegenstehen.

**Kein manueller Abnahmestopp zwischen Phasen.** Fehlen
Zertifikate, API-Verträge, Signaturen, Serverrechte oder
Live-Testergebnisse, dies klar als Blocker führen und
soweit möglich unabhängige Arbeiten fortführen.
Kein Anspruch auf Arbeit im Hintergrund zwischen Chats.

Nur bestehende Projektdateien direkt überarbeiten,
ohne neue Dateien oder redundante Kopien; zu jedem
20er-Block veraltete Pfade und Code kontrollieren.

## Arbeitsregeln

- Nur ogl-viewer auf Branch ogl-develop bearbeiten, nicht das ältere
  Repo nexverse-viewer.
- Zuerst git status und letzten Commit prüfen.
- /goal phasenweise, jeweils 20 Entwicklungsblöcke.
- Ergebnisse testen und direkt auf ogl-develop veröffentlichen.
- Linux, Windows und macOS, AMD/Intel x86-64 und ARM64 bedenken.
- CPU-/GPU-Erkennung und OpenGL-Standardpfad für ältere Hardware.
- Sichtbares Branding/Navigation/Login vollständig OGL/NexVerse,
  kein sichtbarer Firestorm-Altstil; Copyright/Lizenzen erhalten.
  Die bereits installierte originale Firestorm-Skin-/Farbwahl
  ist bisheriger Implementierungsstand, nicht das neue UI-Ziel.
- OpenGL bleibt Standard für ältere Hardware; zusätzlich Vulkan
  als vollständige anwählbare Grafik-Engine entwickeln,
  separate GPU-abhängige RT-Effekte und sichere Rückfallfunktion.
- Login optisch modernisieren und "Vulkan RT bereit" / "Effekt aus"
  aus der Loginoberfläche entfernen.
- GitHub-Releases nach erfolgreichem CI-/Signaturprozess
  automatisch bereitstellen; Stadt-NexVerse-Portal liest
  passende stabile Releases von GitHub, mit Cache/Integritätsprüfung.
- Keine neuen Projektdateien ohne explizite Zustimmung.
  Vorhandene Dateien in-place ändern, keine *_neu/_v2/.bak/
  Kopie-Dateien. Bei jeder 20-Block-Phase Altbestand,
  verwaiste Ressourcen, Duplikate und toten Code prüfen
  und sicher bereinigen, Nutzerdaten/Lizenzen erhalten.
- Feste Grid-Auswahl: Stadt NexVerse und Stadt NexVerse Beta.
- Beta-Anmeldung bleibt bis zur bestätigten HTTPS-/Protokoll-
  Unterstützung gesperrt.
- Verbindlich geplant: ein Pflicht-Updater, der vor der
  Anmeldung prüft, ob die Server-Mindestversion erfüllt ist.
  Veraltete Versionen erhalten keine Serveranmeldung, wenn
  das neue Update noch nicht installiert wurde.

**Letzte unabhängige P02-Vorarbeit (02.11.2026):**
`912a7320` aktualisiert die Einstellung des eingebetteten
Browsers auf offizielle HTTPS-NexVerse-Hosts mit exakter
Host-/Portprüfung und Bewahrung der Nicht-OGL-Policy.
Zehn existierende Sprach-Overlays + zwei Regressionstests.
Linux-Vollbuild, 222 Python-/41 native Tests und isolierter
EN-Xvfb-GUI-Lauf mit Exit 0 bestanden.
**P01 bleibt 18/20, P02 formal 0/20; nächster Block P01 19/20.**
Details in `ogl/PROJECT_STATUS.md`.

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
Beginne mit der im Projektstatus markierten operativen
/goal-Phase P01–P13 und arbeite alle 20 Blöcke ab.
Nach erfolgreichem technischem Gate Abschluss melden und
ohne manuelle Abnahme zur nächsten Phase wechseln.
Bestehende Dateien am bisherigen Pfad ändern;
keine neuen Projektdateien oder Duplikate ohne Freigabe.
Zu jeder Runde Altbestand prüfen und sicher bereinigen.
Linux, Windows und macOS sowie AMD/Intel/ARM64 beachten.
OGL/NexVerse als vollständig eigenes sichtbares UI mit
moderner Navigation/Loginmaske; keine Firestorm-Altoptik.
Aus dem Login "Vulkan RT bereit" und "Effekt aus" entfernen.
OpenGL als Fallback und zusätzliche frei auswählbare
Vulkan-Grafik-Engine inklusive Hardwareprüfung entwickeln.
GitHub-Releases mit signierten Installern automatisch
bereitstellen und vom NexVerse-Portal beziehen.
Beide feste NexVerse-Grids erhalten. Ein verpflichtender
Viewer-Updater muss vom Viewer und Server unterstützt werden;
alte, nicht mehr zugelassene Versionen dürfen sich nicht
anmelden. Änderungen testen, direkt auf GitHub pushen,
den dokumentierten Projektstatus aktualisieren und
die nächsten offenen Schritte nennen.
