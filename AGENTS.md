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
sind P01–P13 im Hauptprojektplan. Aktuell P01 Block 19/20 (1–18 abgeschlossen;
Code `b2520078`: Augenknopf
darf gespeicherte Credential-
Anzeigefüllzeichen nicht wie
echte Passwörter aufdecken;
erst manuelle Eingabe
aktiviert die Reveal-Tasten,
leeres Feld maskiert erneut.
20/20 technische Einzelchecks,
220 Python-/41 native Tests,
DE-GUI-Start und EN-
Screenshot verifiziert,
Beta-TLS weiterhin ungültig.
P02 formal 0/20; echter
Live-Login sowie volle
Windows-/macOS-Viewer-
Abnahmen offen),
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
Zusätzliche unabhängige P02-Vorarbeit vom 02.11.2026:
`912a7320` (OGL-interner Browser für offizielle HTTPS-NexVerse-
Seiten, aktualisierte Sprachoptionen); Linux-Vollbuild,
222 Python-/41 native Tests und isolierter EN-GUI-Start bestanden.
**Formaler Stand bleibt P01 18/20, P02 0/20.** Verbindliche
Detailnachweise stehen in `ogl/PROJECT_STATUS.md`.

Unabhängige P02-Vorarbeit vom 02.11.2026: f1552bb1
blockiert den alten Firestorm-/BugSplat-Crashupload unter
OGL an Linux-/Windows-/macOS-Einstiegspunkten und direkt
im Crash-Logger. Linux-Vollbuild, 227 Python-/41 native
Tests, DE-/EN-Voranmelde-GUI erfolgreich; vollständige
Windows-/macOS-Abnahmen bleiben offen. P01 18/20,
P02 0/20 formal unverändert. Details in PROJECT_STATUS.md.

Unabhängige P02-Vorarbeit (02.11.2026): bf031d29
entfernt Firestorm-Wiki-Hilfelinks aus 13 Setup-
Sprachlayouts, alte Chat-Doku-Links in fünf Sprachen,
sperrt Second-Life-E-Mail-Kontoverweise in OGL.
Linux-Vollbuild, 230 Python-/41 native Tests,
DE-/EN-Xvfb-GUI und 18 XUI-Stagingdateien erfolgreich.
Formaler Stand bleibt P01 18/20, P02 0/20.
Details/weitere Blocker in ogl/PROJECT_STATUS.md.

Unabhängige /goal-20er-Runde (02.11.2026, ef3dc0e5):
Veraltetes Second-Life-DeprecatedHelp-Untermenü
aus EN und elf lokalisierten bestehenden Menüs
entfernt, OGL-Hilfe und Scripting erhalten.
Windows-cp1252-Testfehler durch UTF-8-Dekodierung
repariert; Linux-Vollbuild, 232 Python-/41 native
Tests, 80 Branding-Tests auch mit ASCII, 13 XUI-
Stagingdateien und DE-/EN-GUI-Smokes bestanden.
Formal P01 18/20, P02 0/20, weitere Abnahmen offen.

Unabhängige P02-Vorarbeit vom 02.11.2026:
6b6779fc: aktiver FS-Systemchat, Nearby-Toasts
und Notification-Listen mit bestehendem OGL-Logo,
Demo-Name/Zeit aus Chat-XUI entfernt; Radar-/RLVa-
und Upstream-Fallback behalten. Linux-Vollbuild,
237 Python-/41 native Tests, 85 Branding-Tests,
Ressourcen-Staging und DE-/EN-GUI-Smokes grün.
20 technische Prüfpunkte dokumentiert.
Formal P01 18/20, P02 0/20; authentifizierter
Live-Login und Windows-/macOS-Viewer-Gates offen.

Unabhängige P02-/goal-Runde (02.11.2026):
bde46c18: Chat-Timestamp auf festes rechtes
Feld mit Ellipsen/Tooltip begrenzt; kein
dynamisches Avatar-/Namen-Layout mehr, alte
Demo-Zeiten in DE/JA/PT/TR gelöscht, C++-Dead
Pointer entfernt. 20 technische Prüfpositionen,
240 Python-/41 native Tests, 88 Branding-Tests,
Linux-Vollbuild, sieben XUI-Staging-Dateien und
DE-/EN-Voranmelde-GUI-Smokes bestanden.
Formal P01 18/20 und P02 0/20 unverändert;
authentifizierte In-World-/Plattformabnahmen
weiter offen. Details im Projektstatus.

Unabhängige P02-Vorarbeit (02.11.2026,
cfa671e4): Im OGL-Kanal automatische
Firestorm-Supportgruppen-Metadatenpräfixe
sowie Legacy-Sysinfo-/Versionsdialoge und
auch verzögerte Bestätigungen gesperrt.
Normale IM-Sendewege erhalten.
20 technische Prüfpositionen, Linux-Vollbuild,
245 Python-/41 native Tests, 93 Branding-Tests
und DE-/EN-Xvfb-Voranmelde-GUI bestanden.
Formal P01 18/20, P02 0/20; keine
authentifizierten In-World- oder vollständigen
Windows-/macOS-Vollviewer-Gates behaupten.

Unabhängige P02-Vorarbeit (02.11.2026,
8deebedc): Negative Unread-Zähler in Nearby-/
Muted-/IM-Chat auf null begrenzt; Sprach-
Layouts und Ausgangssichtbarkeit erhalten.
20 technische Prüfpositionen, Linux-Vollbuild,
250 Python-/41 native Tests, 98 Chat-/Branding-
Tests und DE-/EN-Xvfb-Login-Smokes grün.
Beta-TLS-Zertifikatsfehler weiter offen.
Formal P01 18/20, P02 0/20, vollständige
In-World-/Windows-/macOS-Gates weiterhin offen.

Letzte unabhängige P02-Vorarbeit (02.11.2026,
a462330e): Alten FSChatHistoryHeader-
Draw-Resize entfernt; die XUI reserviert
bereits die Zeitstempel-Spalte.
Überholte Namensbreiten-Zustände und
Aufrufe beseitigt, Avatar-/RLVa-/
Zeit- und Teleport-Funktionen erhalten.
20 Prüfpositionen, Linux-Vollbuild,
253 Python-/41 native Tests, 101
Chatlayout-Tests und DE-/EN-Xvfb-
Login-GUI-Smokes bestanden. Formal
P01 18/20, P02 0/20; In-World-/
Windows-/macOS-Vollviewer-Gates offen.

Letzte unabhängige P02-/goal-Runde
(02.11.2026, 72c8beed):
Neun existierende Sprachstrings auf
OGL Viewer und dynamischen
NexVerse-Gridnamen bereinigt;
kein neues Locale/Asset erzeugt.
20 technische Prüfschritte,
Linux-Vollbuild 7.2.5.170,
256 Python-/41 native Tests,
104 Branding-Tests, 13 bytegleich
staged Locale-Dateien und vier
ES/RU/TR/ZH-GUI-Starts bestanden.
Formal P01 18/20, P02 0/20;
Live-Login/In-World/Windows-/macOS-
Abnahmen unverändert offen.

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
