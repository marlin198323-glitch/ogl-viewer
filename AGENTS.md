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

**Operativer /goal-Ablauf:** Die einzig gültige Arbeitsreihenfolge
sind P01–P13 im Hauptprojektplan. Aktuell P01 Block 19/20 (1–18 abgeschlossen; Code-Commit `15162927`, 20/20 Login-Kontotrennung-/Eingabeprüfungen bestanden; echter Hauptgrid-Live-Login und volle Plattform-Builds noch offen),
danach den nächsten Block aus ogl/PROJECT_STATUS.md übernehmen.
Alte Phasen A–I sind fachliche Referenzen, keine separate
Bearbeitungsreihenfolge. Jede Phase in 20 realen Schritten
bearbeiten, betroffene Tests/Builds/CI prüfen, direkt auf
origin/ogl-develop pushen, Status/Commit erfassen und
den Abschluss sichtbar melden. **Keine manuelle Nutzerabnahme
als Phasengate; nach erfolgreichen Checks ohne Rückfrage
mit der nächsten Phase fortfahren, solange die Unterhaltung
aktiv ist und keine echten Abhängigkeiten blockieren.**
Bei Chatende den nächsten Block dokumentieren; keine
asynchrone Fortsetzung ohne aktive Aufgabe versprechen.
Server-/Portal-Schreibzugriffe nur auf verifizierte
Repositories und mit tatsächlichen Berechtigungen.

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

Verbindliche neue Produktgestaltung: Im regulären Nutzerablauf
darf **nichts vom bisherigen Firestorm-Branding oder der alten
Firestorm-UI sichtbar sein**. OGL/NexVerse benötigt eine eigene
Navigation, moderne Loginmaske, Fenster, Chat, Texte und Icons.
Die Anzeigen "Vulkan RT bereit" und "Effekt aus" sind aus
dem Login zu entfernen. Technische RT-Diagnosen gehören in
Grafikeinstellungen/Logs; rechtlich nötige Copyright- und
Lizenzinformationen dürfen nicht entfernt werden.

Grafik-Engine-Auswahl verbindlich entwickeln: OpenGL als
Standard/Fallback für ältere Hardware; Vulkan als zweite
vollwertige Renderer-Engine, anwählbar in Grafikeinstellungen.
Vulkan-RT-Effekte sind separat und nur bei unterstützter
GPU verfügbar. Bisherige Vulkan-RT-Prototypen sind
**noch kein** auswählbarer vollständiger Vulkan-Welt-Renderer.

Github Releases soll nach erfolgreich geprüften Builds
signierte OGL-Installationspakete für die unterstützten
OS-/CPU-Varianten automatisch bereitstellen. Das
NexVerse-Portal bezieht Releases aus offizieller GitHub-
Quelle mit Caching und Integritätsprüfung. Pflichtupdate-
Sperre muss zusätzlich auf dem Server durchgesetzt werden.
**Dies ist geplant, nicht fertig implementiert.**

**Strenge Dateiregel:** Bestehende Dateien am gleichen Pfad
bearbeiten/überschreiben, niemals *_neu, *_v2, *.bak,
Kopien, parallele Templates oder zusätzliche Projektdateien
anlegen. Ist eine neue Projektdatei aus technischen Gründen
unvermeidlich, vorab ausdrücklich um Freigabe bitten.
Von CI erzeugte Release-Artefakte außerhalb des Quellbaums
sind davon getrennt. Zu jeder /goal-Runde eine prüfbare
Altbestand-/Duplikatkontrolle durchführen und ungenutzten
Code erst nach Abhängigkeits-/Lizenz-/Regressionstests
entfernen. Keine Nutzerdaten oder Secrets blind löschen.
Plugins und Theme-Installationen sicher halten.

Nach Änderungen Tests/Builds dokumentieren, nach
erfolgreicher Prüfung auf origin/ogl-develop pushen
und ogl/PROJECT_STATUS.md sowie die Roadmap aktualisieren.
Keine Aussage über vollständige Windows-/macOS-Releases
allein aus Modul-CI-Tests ableiten.
