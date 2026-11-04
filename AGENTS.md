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

**Operativer /goal-Ablauf:** P01–P13 in der Hauptroadmap.
P01 (Linux) ist seit 02.11.2026 mit 20/20 abgeschlossen.
Nutzerbestätigte Live-Tests: Anmeldung erfolgreich,
3D-Welt funktioniert, sauberer Viewer-Shutdown,
gespeicherte Logindaten werden wiederhergestellt.
Das sind Nutzer-Abnahmeaussagen, keine automatisch
authentifizierten Tests des Assistenten.
Linux-Vollbuild 7.2.5.180, 270 Python-/41 native Tests
waren unabhängig zuvor bestanden.
Windows-/macOS-Vollviewer-GUI und -Build sind
noch nicht testbar, bleiben in P13 als Pflicht
vor Freigabe für diese Systeme vorgemerkt.
Beta-TLS-Zertifikatsabweichung bleibt P08.
P02 Blöcke 1–20 Linux-Code-/Regressions-
Phase abgeschlossen (abschließender
Commit 68e50e2b, Viewer 7.2.5.200,
299 Python-/41 native Tests,
DE-/EN-GUI-Smokes). Live-In-World-
Chat, Voice, DPI und Windows/macOS-
Vollviewer bleiben P13-Gates.
Aktive Phase P05 Block 3/20:
P03/1–8 c4a615ba,
9–15 73565590,
16 1f174bc7,
17 59bc92d3.
P03/17 erstmals echter
nativer .ogltheme-
Install-/Entfernungs-
End-to-End-GUI-Erfolg
im Linux-Viewer
7.2.5.210:
Ocean-Blue-Theme mit
GTK-LLFilePicker
explizit ausgewählt,
installiert, Manifest-
Datei kontrolliert,
ohne Aktivierung
wieder entfernt.
Native ZIP-Manipulations-
GUI-Tests für Extra-
Einträge, Symlinks,
../-Pfade und
doppelte JSON-Keys
alle abgelehnt, ohne
Schreibreste, Exit 0.
Ursache des vorigen
Importfehlers: leere,
aber nicht-null ZIP-
linkname-Zeichenkette;
nun sichere Prüfung
auf NICHTLEERE Links
plus Unix-Dateimodus.
Linux-Vollbuild und
312 Python-/41 native
Tests bestanden.
Keine signierten Themes,
keine Bild-Assets,
keine Vollviewer-
Windows-/macOS-Abnahme.
Nächster Block 18:
ungenutzte Skin-Overrides
und Theme-Duplikate
belegt identifizieren,
nur sicher bereinigen,
keine neuen parallelen
Templates/Dateien.
P03 Block 18, Commit
854e8a2d: Acht Skins,
13 Default-Lokalisierungen,
Theme-Sample-IDs
inventarisiert. Genau
eine OGL-Theme-EN-UI
plus DE-Lokalisierung,
keine parallelen
OGL-Panels. Identische
Upstream-Skin-/Locale-
Dateien nicht löschen.
Python-Regression zur
Theme-Template-
Einzigartigkeit ergänzt,
313/313 Tests bestanden.
Nächster Block 19:
Linux-Vollbuild,
native CTests,
portable Theme-CI.
P03 Blöcke 19–20
operativ als Linux-
Code-/QA-Phase
abgeschlossen:
Vollbuild 7.2.5.214,
313 Python-/41 native
Tests; DE/EN-
OGL-Designs-GUI
100/140 % Exit 0.
GitHub graphics-CI
41891918909
Linux/Windows/macOS
x64/ARM je erfolgreich;
Windows-x64-Installer-
Job SKIPPED, somit
kein vollständiger
Windows/macOS-
Viewer nachgewiesen.
P03 Theme-Import
bewusst eingeschränkt
auf unsigned lokale
Farb-Manifest-ZIPs
ohne Assets/Plugins.
Signaturen/Vollviewer-
Abnahme weiterhin
nicht freigegeben;
Gates getrennt.
Ab jetzt P04 Block 1:
Renderer-OpenGL-/
Vulkan-Inventur.
P04 Block 1 Commit
c2aade42 abgeschlossen:
Welt-Renderer ist
OpenGL LLPipeline,
Vulkan/RTX und
ogl_gpu_probe nur
optionale Modul-
Diagnostik, opt-in
Schatten-Compositing
weiter in OpenGL.
Voll-Vulkan-Engine
und UI-Auswahl
fehlen, macOS-
Host Stub. Kein
C++-Renderer geändert.
Bestehende public-
graphics Tests
um Baseline erweitert,
314 Python grün.
P04 Block 2:
Backend-Vertrag/
Lebenszyklus definieren,
GL-Fallback erhalten.
P04/2: Im vorhandenen `indra/newview/oglrtxhost.h` existiert ein rein deklarativer constexpr-Backend-Vertrag. OpenGL bleibt Standard, Vulkan-GPU-Diagnostik berechtigt nicht zum Welt-Renderer. Linux 7.2.5.218 seriell gebaut, 315 Python-/41 native Tests grün. P04/3: bestehende Render-Frame-Pfade unter Erhalt der GL-Funktion verbinden; keine Parallel-Rendererdateien.

P04 Blöcke 3–17 (Code ec52fd45):
Echter GL-Framepfad + Capability/Fallback-Lifecycle in bestehenden
Dateien, EN/DE-Grafik-Reiter mit Vulkan mangels Voll-World-Swapchain
deaktiviert, Requested/LastWorking unabhängig und letzter Erfolg erst
nach realem GL-Swap gespeichert. Linux 7.2.5.219, 319 Python,
41 native CTests, 7 C++-Szenarien, echter DE-GUI- und manueller
Vulkan-Request-Fallback bestanden. P04/18 portable GitHub CI
prüfen; Full-Viewer Windows/macOS NICHT durch Modul-CI ersetzen.
Niemals Vulkan-RTX-Diagnose als 3D-Engine behaupten.
Nächster /goal P04/18. Keine weiteren parallelen Templates.

P04 20/20 als Linux-Code/portable Renderer-Fallback-Phase
abgeschlossen. Code ec52fd45, Viewer 7.2.5.219,
319 Python/41 native Tests und sieben C++-Fallprüfungen.
Vulkan-Request fällt beim echten Linux-Start auf GL
zurück; Grafik-Tab EN/DE zeigt Vulkan als deaktiviert.
Run 41901011126: alle sechs portable Plattformmodul-
Jobs grün. Windows-Installer SKIPPED, nicht vorhanden.
P05/1: Vulkan-Instance/Device/Queue als echte Welt-
Engine aufbauen. Bestehende RTX-Moduldiagnose nicht
mit vollständigem World-/Swapchain-Renderer verwechseln.
Alte Userdaten, Login, GL-Kombatibilität erhalten.

P05 04.11.2026: Geräte-/Queue-Bootstrap unter Opt-in OGL_VULKAN_WORLD_BOOTSTRAP=1 implementiert; herstellerneutral, kein Welt-Rendering. Linux-Build, 321 Python-/41 native Tests, reale OpenGL-GUI-Starts bei Opt-in an/aus bestanden. P05 0/20 bis echte Vulkan-Window-Surface und Swapchain integriert sind. Kein CUDA-/RTX-Zwang und keine Benutzerprofildateien anfassen.

P05 Block 1 abgeschlossen (04.11.2026): Temporäre
echte Vulkan-Xlib-Surface und Swapchain auf DEM
SDL/OpenGL-Viewerfenster unter zweifachem Dev-Opt-in.
Presentqueue sowie Caps/Format/FIFO/Images validiert,
beide Handles unmittelbar vor GL-Frame freigegeben.
Keine Vulkan-Weltframes und kein Engine-Switch.
Linux-Vollbuild 7.2.5.219, 323 Python- und
41 native Tests, echte Xvfb-GUI mit WSI-Pass,
normalem GL und invalidem Vulkan-ICD/Fallback OK.
Nächster P05 Block 2/20: kontrollierter dauerhafter
WSI-Window-/Frame-/Resize-Lifecycle, weiterhin
OpenGL-Fallback und keine Nutzerprofile anfassen.

P05 Block 2 abgeschlossen (04.11.2026):
Persistentes Vulkan-WSI nur auf Linux/X11 und mit drei
Entwickler-Opt-ins; echte SDL-Resize-/Destroy-Callbacks.
Zwei reale GUI-Resizes 1230x820 / 1050x680 lieferten
jeweils swapchain=ready bei weiterhin aktivem OpenGL.
Linux-Vollbuild, 324 Python-/41 native Tests grün.
P05 2/20, nächster Block 3 GPU-Frame-Synchronisation.
Keine vollständige Vulkan-Welt-Engine behaupten.

Verbindlich ist der Anfang von PROJECT_STATUS.md.

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

Letzte unabhängige P01-Sicherheitsrunde
(02.11.2026, 4ff7e7cb): Fehlende gespeicherte
Kontoauswahl bei Startup-Credentials
führt zu Passwort-Neueingabe statt zu
verwendbarem maskierten Fülltext.
Zustände für Login/Augenknöpfe bereinigt,
One-part-OpenSim-Kontoanzeigen ergänzt.
20 Prüfpunkte, Linux-Vollbuild,
259 Python-/41 native Tests, 107
Branding-/49 Gridtests (ASCII) und
DE-/EN-Voranmelde-GUI bestanden.
Keine echten Credentials getestet.
P01 formal 18/20, P02 0/20;
Live-Login und alle Plattform-Gates offen.

Letzte unabhängige P01-/goal-Vorarbeit
(02.11.2026, 8e9e773e): Legacy-Grid-
Hilfe und -Builder auch gegen direkte
UI-Callbacks abgesichert, OGL-Hilfe nur
NexVerse-Portal, kein Fremd-Grid-Builder.
20 Prüfschritte; Linux-Vollbuild 7.2.5.174,
262 Python-/41 CTests, 110 Branding-/
49 Gridtests, 23 Login-Overlay-Prüfungen
und DE-720p-/EN-1080p-GUI bestanden.
Formal P01 18/20, P02 0/20;
Live-Login und volle Plattformgates offen.

Letzte unabhängige P01-/goal-20er-Runde
(02.11.2026, be18b791): Login-Retry-
Zwischenspeicher nur für manuelle
Passwörter; maskierte gespeicherte
Authenticator nicht als Klartext
wiederherstellen. Beim Konto-
Wechsel altes Klartext-Retry
verwerfen, fehlende manuelle
Eingabe erzwingt Passwortneueingabe.
Linux-Vollbuild 7.2.5.176,
265 Python-/41 native Tests,
113 Branding-/49 Gridtests,
DE-720p-/EN-1080p-GUI grün.
20 technische Prüfschritte; formal
P01 18/20, P02 0/20, Live-
und Plattform-Gates noch offen.

Letzte unabhängige P01-/goal-Runde
(02.11.2026, bfd5e2d9): Beim
OGL-Benutzernamenwechsel Passwort-/
Retry-Zustand auch bei bereits leerem
Feld löschen, Augenknöpfe für unbekannte
Konten synchronisieren. 20 Prüfschritte,
Linux-Vollbuild 7.2.5.178,
268 Python-/41 CTests, 116 Branding-
/49 Gridtests und DE-/EN-GUI grün.
Formal P01 18/20, P02 0/20;
Live-/Plattform-Gates offen.

P01 Block 19 technische Vorprüfung
(02.11.2026, 263a224e): Main-Grid-
GET /get_grid_info HTTP 200/XML,
Beta-TLS-Zertifikat weiter fehlerhaft.
Credential-Reuse darf nur einen
@-Trenner enthalten, zehn native
Policy-Fälle ergänzt. P01-Grid-/Login-
/Startup-Änderungen triggern jetzt
sechs portable Plattform-CI-Jobs.
Linux-Vollbuild 7.2.5.180,
270 Python-/41 CTests, 116 Branding-
/51 Gridtests, DE-/EN-GUI bestanden.
20 Checks; keine Live- oder vollständige
Windows-/macOS-Viewer-Abnahme.
Formal P01 18/20, P02 0/20.

Windows-Testinstaller (02.11.2026):
Der vorhandene aktive OGL-portable-graphics-
Workflow hat einen separaten Windows-2022-
Vollviewer-Testjob, der ausschließlich bei
Push-Committext [windows-installer] startet.
Er soll ein unsigniertes NSIS-Setup als
GitHub-Artefakt erzeugen, ohne Releases und
sechsfache Modul-CI zu verändern. Vorerst
keinen funktionierenden Windows-Installer
behaupten, bevor der Windows-Build tatsächlich
erfolgreich ist. P01 Linux 20/20 und P02 1/20
bleiben unverändert. Windows-Freigabe P13.

P02 Navigation (02.11.2026, 9c10de4e):
Hauptmenüreihenfolge Me/Communicate/
World/BuildTools/Content/Help ist
verbindlich für alle 13 Sprachlayouts;
RLVa/Advanced/Develop/Admin bleiben
erhalten. Spanische V1-Altmenüs entfernt,
DA/TR Content-Labels lokalisiert.
Linux-Vollbuild 7.2.5.186,
276 Python-/41 native Tests, ES/DA/TR-
Prelogin-GUI, 3/3 staged Menüdateien
bestanden. Formal P02 2/20;
Block 3 ist der nächste Schritt.
Keine neuen Paralleltemplates erstellen.

P02 Block 3 (02.11.2026, 4a2f34cc):
Admin-Status-Befehle nur im
Entwickler-Menü, aus elf Avatar-
Sprachlayouts entfernt. EN-Toolbar-
Basis und zehn Sprach-Overlays
auf Eindeutigkeit geprüft;
Bewegungs-Toggle-Shortcuts erhalten.
Linux-Vollbuild 7.2.5.188,
279 Python-/41 native Tests,
125 Brandingtests, 13/13 staged
Menüs und DE-/EN-/ES-GUI grün.
P02 3/20; als Nächstes Block 4.
Windows-Installer separat weiter
nicht als fertig behaupten.

P02 Block 4 (04.11.2026, fc097c18):
Spanisches Entwicklermenü um
doppeltes DevelopWorld und leeres
Set Logging Level bereinigt. Leere
DE/PT/ZH-toolbar_panel-Overlays
entfernt, lokale Chatbuttons erhalten.
Vollviewer 7.2.5.190, 282 Python-/
41 native Tests, 128 Brandingtests,
13/13 Menü- und 11/11 Toolbar-XUI,
vier Stagingdateien und vier GUI-
Smokes erfolgreich. P02 4/20,
nächster Block 5. Windows-Installer
nicht verfügbar: NSIS-Output im CI-
Log, Artefakt-Upload fehlgeschlagen.

P02 Block 5 (04.11.2026, 8c274734):
OGL-kanalspezifisches "Über"-Fenster
mit bestehendem OGL_System_Icon,
GitHub-Supportziel, originalem
Copy-Handler und lokalisiertem
Buttontext in 13 Sprachen.
Firestorm-/Linden-Credits und
Nicht-OGL-Verhalten unverändert.
Linux-Vollviewer 7.2.5.192,
285 Python-/41 native Tests,
131 Brandingtests, 14 XUI-
Stagingvergleiche und DE-/EN-
Prelogin-GUI bestanden. Kein
echter About-In-World-Klick.
P02 5/20; nächster Block 6.
Windows-Installer noch nicht
herunterladbar (Upload scheiterte).

P02 Block 6 (04.11.2026, 8960ecec):
Im vorhandenen Chat-Header
japanischen Test-Namen und
spanischen Test-Label entfernt;
erweiterter Chat-Zeitstempel
befolgt show_time, Teleport-Trenner
ohne Zeit. Asynchrone Avatar-
Namensantworten mit fremdem
agent_id dürfen nicht überschreiben.
RLVa/Avatar-Icons intakt.
Linux-Vollbuild 7.2.5.194,
288 Python-/41 native Tests,
134 Brandingtests, 7 Chat-
Header-XML und 2 Paketdateien,
JA-/ES-/DE-GUI-Smokes bestanden.
P02 6/20, nächster Block 7.

P02 Block 7 (04.11.2026, b04c547c):
Aktive IM-Zeile ohne Demo-Name;
sofort echte P2P-/Gruppen-/Ad-hoc-
Sitzungsnamen, P2P-Avatar-Cache
mit Teilnehmer-ID-Prüfung und
Signal-Disconnect bei Zeilenende.
Einzelchat-Avatarprofil 104x104
rund, Gruppenliste unverändert.
Linux-Vollbuild 7.2.5.196,
291 Python-/41 CTests,
137 Brandingtests, zwei XUI-
Paketvergleiche und DE-/EN-
GUI-Smokes bestanden.
Keine In-World-IM-Abnahme.
P02 jetzt 7/20; Block 8 folgt.

P02 Block 8 (04.11.2026, 8116671a):
IM-Tabwechsel markiert nur sichtbare
ausgewählte Sitzung gelesen; alle
anderen Unread-Statusmarkierungen
bleiben. Tab-Animation gezielt
stoppen, verstecktes/minimiertes
Fenster nicht automatisch
anerkennen. Ad-hoc-Session-ID-
Migration inklusive Markierungen
und Close-Signal per Floater statt
alter UUID; Voice-Status-Zeiger
beim Entfernen neu setzen,
Toolbarzugriff gegen Null geprüft.
Linux-Vollbuild 7.2.5.198,
295 Python-/41 native Tests,
141 Brandingtests, DE-/EN-
Prelogin-GUI bestanden.
Kein Live-In-World-IM/Voice-
Test. P02 8/20, nächster Block 9.

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
