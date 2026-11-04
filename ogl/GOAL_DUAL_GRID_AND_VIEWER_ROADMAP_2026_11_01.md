# /goal – OGL Firestorm: plattformübergreifender Entwicklungsplan

Stand: 01.11.2026 | Hauptbranch: ogl-develop

## Verbindlicher Projektauftrag – bei jedem neuen Chat zuerst lesen

**Kanonisches GitHub-Repository:** https://github.com/marlin198323-glitch/ogl-viewer
**Arbeitsbranch:** ogl-develop
**Linux-Mint-Rechner / Commander:** jam-MS-7D28
**Projektverzeichnis:** /home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm

Dieser Auftrag gilt für sämtliche folgenden /goal-Phasen:

1. **Richtiges Projekt:** Ausschließlich das Firestorm-Fork-Repository
   marlin198323-glitch/ogl-viewer auf ogl-develop bearbeiten;
   nicht das frühere, getrennte Repository nexverse-viewer.
2. **Bestand schützen:** Zunächst git status, den letzten Commit,
   die Projektübergabe und Roadmaps lesen. Keine fremden Änderungen,
   lokale Nutzerdaten, Einstellungen oder Zugangsdaten überschreiben.
3. **Arbeitsweise:** Über den Linux-Mint-Commander arbeiten.
   Jede /goal-Phase in **20 konkreten Entwicklungsblöcken** planen
   und tatsächlich abarbeiten. Teilweise fertige Phasen als
   **in Arbeit** kennzeichnen, niemals vorschnell als abgeschlossen.
4. **Direkt freigeben:** Implementierung testen, den echten Viewer
   soweit relevant bauen, Testresultate dokumentieren und geprüfte
   Änderungen direkt auf origin/ogl-develop veröffentlichen.
   Plattformen, die noch nicht geprüft wurden, offen benennen.
5. **Gemeinsamer Viewer:** Linux, Windows und macOS sowie
   AMD/Intel x86-64 und ARM64 berücksichtigen. CPU-/GPU-Funktionen
   zur Laufzeit erkennen; ältere Hardware muss weiterhin über einen
   funktionsfähigen OpenGL-Standardpfad arbeiten können.
6. **Grid-Auswahl:** Genau zwei fest eingebaute Ziele behalten:
   Stadt NexVerse (bestehendes OpenSim) und
   Stadt NexVerse Beta (https://beta.stadt-nexverse.de).
   Die Beta darf erst nach verifizierter TLS- und
   Protokollunterstützung Zugangsdaten übertragen.
7. **Pflicht-Updater:** Eine neue, vom Server nicht mehr akzeptierte
   Viewer-Version muss aktualisiert werden, **bevor** Zugang zu
   einem der beiden Grids gewährt wird. Ablehnen/Abbrechen des
   Updates darf keinen Server-Login ermöglichen. Die
   Mindestversionsprüfung ist **auch serverseitig** durchzusetzen,
   nicht nur im Viewer. Sie ist derzeit geplant, nicht fertig.
8. **OGL/NexVerse als eigenständiger Viewer:** Die reguläre,
   sichtbare Oberfläche – Logo, Navigation, Login, Fenster,
   Chat, Menüs, Texte und Dialoge – trägt ausschließlich das
   neue OGL-/NexVerse-Design. Kein sichtbares Firestorm-Branding
   oder veraltete Firestorm-UI-Elemente im Nutzungsablauf.
   Ein interner, nicht sichtbar angebotener OpenGL-/Skin-
   Kompatibilitätsfallback bleibt möglich. Lizenzhinweise,
   Urhebervermerke und rechtliche Attributionen müssen erhalten
   bleiben. Installierbare Designs bleiben sicher und rücksetzbar.
9. **Grafik-Engines:** Der Nutzer soll in den Grafikeinstellungen
   zwischen **OpenGL (Standard/Kompatibilität)** und **Vulkan
   (zusätzliche vollständige Grafik-Engine)** wählen können.
   Vulkan-Raytracing ist eine getrennte optionale Funktion und
   kein Synonym für Vulkan-Rendering. Unterstützte GPU,
   Treiber, Betriebssystem und API-Funktionen zur Laufzeit prüfen;
   bei inkompatibler Hardware/Startfehler automatisch nach
   OpenGL zurückfallen. Enginewechsel bei Bedarf erst nach
   kontrolliertem Viewer-Neustart.
10. **Login und Navigation modernisieren:** Eigenständige,
    übersichtliche, responsive OGL-Navigation. Loginbereich
    kompakter und moderner, Felder lesbar, Grid-Auswahl und
    Passwortspeicherung weiterhin korrekt bedienbar. Die
    Anzeigen **„Vulkan RT bereit“** und **„Effekt aus“**
    vollständig aus dem regulären Login entfernen; technische
    GPU-/RT-Informationen ausschließlich in optionale
    Diagnose-/Grafik-Einstellungen oder Logs verlagern.
11. **GitHub-Releases und Portal:** GitHub Releases im
    kanonischen Repository sind der Veröffentlichungsort
    für signierte/versionierte OGL-Installationspakete.
    CI erstellt und veröffentlicht die passenden Builds nach
    erfolgreichen Tests. Das NexVerse-Portal bezieht die
    aktuellen Releases über die offizielle GitHub-Releases-API
    beziehungsweise ein daraus generiertes, geprüftes
    Release-Manifest (mit serverseitigem Cache/Rate-Limit-
    Schutz). Downloads werden nach OS und CPU ausgewählt;
    keine erfundenen Download-URLs und keine unsignierten
    automatischen Installationen. Die **serverseitige**
    Mindestversion aus Phase F bleibt verbindlich.
12. **Bestehende Dateien statt Dateiduplikate:** Bei jeder
    Änderung vorhandene Quell-, Layout-, Template-, CSS- und
    Konfigurationsdateien **am bisherigen Pfad bearbeiten**.
    Keine parallelen *_neu-, *_v2-, Kopie-, Backup- oder
    überlagernden Ersatzdateien und **keine neuen Dateien
    im Projekt** ohne vorherige ausdrückliche Freigabe.
    Wenn technisch eine neue Datei unvermeidbar wäre, den
    Blocker zuerst dokumentieren und die Zustimmung einholen.
    Von CI erzeugte Release-Artefakte außerhalb des
    Quellbaums sind hiervon zu unterscheiden.
13. **Regelmäßige Projektbereinigung:** In jeder 20-Block-
    Phase alte/ungenutzte Quellpfade, verwaiste Ressourcen,
    doppelte Templates, toten Code, überholte Verweise und
    nicht genutzte Artefakte untersuchen, aber nur nach
    Abhängigkeits-, Lizenz- und Regressionstest entfernen.
    Keine Secrets, .env, Nutzerdaten, existierenden
    Benutzerprofile oder caches blind löschen.
    Git-Verlauf, Upstream-Lizenzen und reproduzierbare
    Builds bewahren.
14. **Projektgedächtnis im Repository:** Nach jedem bearbeiteten
   /goal-Block den Status in
   [PROJECT_STATUS.md](PROJECT_STATUS.md) fortschreiben.
   Beim Phasenabschluss Roadmap, Tests, offene Blocker und
   GitHub-Commit aktualisieren. Diese Dokumente sind die
   Übergabequelle für spätere Chats.

**Lese-Reihenfolge für neue Chats / Entwicklungsassistenten:**
AGENTS.md im Repo-Hauptverzeichnis →
[PROJECT_HANDOFF.md](PROJECT_HANDOFF.md) →
[PROJECT_STATUS.md](PROJECT_STATUS.md) →
dieser Projektplan → Detailpläne der aktiven Phase.

Ein neuer Chat kann den Inhalt nicht allein wegen einer neuen
Unterhaltung automatisch kennen; deshalb zuerst dieses
GitHub-Repository und die genannten Dateien öffnen.

## Grundsätze

- Ein Firestorm-basierter Quellcode für Linux, Windows und macOS.
- x86-64 (AMD/Intel) und ARM64 mit CPU- und GPU-Fähigkeitserkennung.
  Für ältere Hardware muss der Standard-OpenGL-Pfad weiter funktionieren.
- OGL/NexVerse ist das alleinige sichtbare Produktdesign;
  der ursprüngliche Firestorm-Stil bleibt höchstens interne
  technische Rückfallbasis, inklusive nötiger Lizenzhinweise.
- OpenGL ist der Kompatibilitäts-Standard; Vulkan wird als
  eigenständig auswählbare Renderer-Engine entwickelt.
- Jede /goal-Phase in 20 einzeln überprüfbaren Blöcken.
- Keine unbestätigten Protokolle oder unbekannten Server für Zugangsdaten.

## Operativer /goal-Phasenlauf – verbindliche Ausführungsreihenfolge

Diese Reihenfolge **ersetzt** die bislang alphabetisch gegliederten
Fachphasen A–I als Arbeitsablauf. Die Fachphasen bleiben darunter
als technische Referenz erhalten; bei Widersprüchen hat dieser
operative Phasenlauf Vorrang.

**Ausführung ohne manuelle Zwischenabnahme:**
- Kommando `/goal` heißt: an der im Projektstatus dokumentierten
  nächsten offenen Phase/Blocknummer fortfahren, in 20er-Blöcken
  implementieren, testen, auf `origin/ogl-develop` veröffentlichen.
- Eine Phase ist **nur fertig**, wenn ihre technischen Abnahmekriterien
  bestanden sind. Dazu gehören betroffene Unit-/Integrationstests,
  notwendige Linux-Viewer-Builds und die passende Plattform-CI.
  Werterfassung oder sichtbare UI-Smoketests automatisieren, soweit
  möglich; keine obligatorische manuelle Benutzerfreigabe zwischen
  Phasen. Eine noch nicht überprüfte Live-Funktion bleibt ehrlich
  **offen**, nicht künstlich „bestanden“.
- Beim Abschluss sofort mitteilen:
  `/goal Pxx abgeschlossen | 20/20 | Tests: ... | GitHub: <SHA>`
  sowie `Nächste Phase: Pyy`. Während derselben aktiven
  Bearbeitung ohne Rückfrage mit Pyy fortfahren, sofern technisch
  ausführbar und keine sicherheitsrelevanten Freigaben fehlen.
- **Keine unbeaufsichtigte Hintergrundfortsetzung behaupten**:
  Außerhalb einer aktiven Unterhaltung wird nicht selbstständig
  weiterentwickelt. Bei Chatende schreibt der Assistent den
  genauen nächsten Block in `ogl/PROJECT_STATUS.md`. In einem
  neuen Chat reicht `/goal` plus Zugriff auf das Repository.
- Bei einem echten Blocker (z. B. unzugängliches Backend,
  fehlender Signaturschlüssel, defekte CI oder fehlendes
  Protokoll) die betroffene Phase als **blockiert** melden,
  Ursache dokumentieren und ausschließlich unabhängige
  bereits geplante Arbeiten fortsetzen. Niemals Sicherheits-
  gates umgehen, ungetestete Builds als fertig deklarieren
  oder blind auf Produktivsystemen veröffentlichen.
- **Dateiregel:** Vorhandene Projektdateien am bestehenden Pfad
  ändern. Keine neuen Dateien, Duplikate, .bak/_neu/_v2
  oder parallele Implementierungen ohne ausdrückliche
  Zustimmung. Wenn eine neue Quelldatei unvermeidbar wäre,
  als Blocker kennzeichnen, statt die Vorgabe heimlich
  aufzuheben. CI-Releases außerhalb des Quellbaums sind erlaubt.
  Pro Phase Altbestand prüfen und nach Abhängigkeits-,
  Lizenz- und Regressionstests gezielt bereinigen.
- Phasen auf Linux Mint erarbeiten, aber Code/Distribution
  auf Linux, Windows, macOS und x64/ARM64 abstimmen.
  Das öffentliche OpenSim-Grid darf nicht durch Beta-
  Experimente oder Vulkan-Änderungen ausfallen.
- Externe NexVerse-Server-/Portal-Repositories sind eigene
  Änderungen: tatsächliches Zielrepository und erforderliche
  Zugriffsberechtigungen vor Schreibzugriff verifizieren.
  Ohne passende Berechtigung wird die Integration als
  blockiert markiert und kein fremdes Repository vermutet.

### Phasenübersicht – 13 mal 20 Entwicklungsblöcke

| Phase | Reihenfolge | Schwerpunkt | Status |
| --- | --- | --- | --- |
| P01 | 1 | Branding und modernes Login | **Aktiv – 18/20** |
| P02 | 2 | OGL-Navigation, Fenster, Chat und Avatare | **Geplant** |
| P03 | 3 | Theme-System und sichere Designinstallation | **Geplant** |
| P04 | 4 | Renderer-Abstraktion und OpenGL-Fallback | **Geplant** |
| P05 | 5 | Vulkan als vollständige auswählbare 3D-Engine | **Geplant** |
| P06 | 6 | Vulkan-Raytracing und Grafikqualität | **Geplant** |
| P07 | 7 | CPU/GPU-Leistung, Textur-, Mesh- und Avatarcache | **Geplant** |
| P08 | 8 | Beta-Protokoll und getrennte NexVerse-Anmeldung | **Geplant** |
| P09 | 9 | Abgesicherter Plugin-Manager | **Geplant** |
| P10 | 10 | Automatisierte signierte GitHub-Releases | **Geplant** |
| P11 | 11 | Clientseitiger Pflicht-Updater und Update-UI | **Geplant** |
| P12 | 12 | Serverseitige Mindestversion für beide Grids | **Geplant** |
| P13 | 13 | Portal-Releases, End-to-End-QA und Dauerbereinigung | **Geplant** |

**Aktive nächste Phase:** P01, Block 19/20. Die früheren
Teilstände (zwei feste Grids, OGL-Farbthemen,
SIMD-/RT-Diagnose) sind keine automatische Freigabe
der neuen operativen Phasen. Fertiggestellte Bestandteile
bei jeder Phase übernehmen und nicht erneut implementieren.

### /goal P01 – Branding und modernes Login

**Status:** **P01 aktiv, 18/20 Phasenblöcke**, derzeit
Block 19 – Grid-Login-/Plattformregression.
Letzter P01-Code-Commit `b2520078`:
OGL-Augenknopf kann bei
gespeichertem Credential
keinen falschen
Passwort-Platzhalter
sichtbar machen.
Erst manuelle Eingabe
schaltet das Auge frei;
Leeren maskiert erneut.
20/20 technische Prüfungen,
220 Python-/41 native
Tests, DE-Linux-GUI
und EN-Xvfb-Screenshot
bestätigt. Main-Gridinfo
noch gültig, Beta-TLS-
Hostnamen-Zertifikat
weiterhin unpassend.
Beta-GUI-Klicktest nicht
als erfolgreich behaupten.
Davor:
`c3db484e`:
OGL maskiert zuvor
sichtbare Passwörter
automatisch nach jeder
Konto- und Gridänderung,
einschließlich getippter
Benutzernamen und gespeicherter
Credential-Wiederherstellung.
20/20 technische Prüfpunkte,
215 Python-/41 native
Tests, DE-/FR-GUI-
Negativstarts bestanden.
Klartext-Augenschalter
bleibt für aktuelle
Eingabe verfügbar.
Davor:
`3a655add`:
Der externe OGL-Welcome-Browser
ist in allen drei vorhandenen
Login-Skins nicht privilegiert.
Der C++-Loader erzwingt
`setTrustedContent(false)`
vor jeder Navigation; drei
zusätzliche Regressionstests,
20/20 technische Kontrollen,
210 Python-/41 native Tests,
EN-/DE-GUI-Starts bestanden.
Portable Grafik-CI 6/6
(Lauf 41479072688) grün.
Keine vollständige Live-Login-
oder Plattform-Viewer-Abnahme.
Davor:
`f76ad6f1`:
Passwortfeld der aktiven
OpenSim-Login-XUI initial
255 statt 16 Zeichen;
feste Gridauswahl und
Beta-Einschränkung in
zehn vorhandenen Sprachen
lokalisiert, alte SL-
Passwortlinks FR/PT/JA
entfernt, bestehende
Starlight-Loginvarianten
synchronisiert. 20/20
technische QA-Positionen,
207 Python-/41 native Tests
sowie echte DE-/FR-GUI-
Voranmeldeläufe erfolgreich;
portable Grafikmodul-CI
6/6 Plattformjobs grün
(Lauf 41476349998).
Dies ist keine vollständige
Live-Login-Abnahme.
Davor:
`7ab16aaf`:
Gepinnte NexVerse-Credentials
dürfen nur bei exakter bewusster
Kontenauswahl auf dem aktuellen
kanonischen Grid wiederverwendet
werden; bloße Eingabe des
Benutzernamens, Gridabweichung,
verändertes Passwort oder alte
Maskierungsfüllzeichen erlauben
keine Secret-Wiederverwendung.
16 native Positiv-/Negativtests,
203 Python-/41 native Tests
und Hauptgrid-/Beta-GUI-Smokes
bestanden; portable Grafikmodul-
CI 6/6 Linux/Windows/macOS
(Lauf 41340630759), **keine**
vollständigen OS-Viewer-Builds;
20/20 technische
Prüfpositionen, aber noch keine
echte Passwortpersistenz-Abnahme.
Davor:
`a8951b29`:
Moduswechsel-Bestätigungen werden
einmalig an Dialoggeneration, aktuelle
Auswahl und unverändertes Original-
Preset gebunden. Sieben vorhandene
alte Login-XUI-Dateien enthalten
keine Phoenix-/Second-Life-Fallback-
URLs mehr. 20/20 QA-Positionen,
198 Python-/41 native Tests,
Hauptgrid-/Beta-GUI-Negativtests
bestanden. CI-Nachfix `0fb7b3d0`:
UTF-8-Decoding in Windows-Python-
Quellextrakttest explizit gemacht;
portable Grafikmodule auf 6/6
Linux-/Windows-/macOS-Matrixjobs
erfolgreich (Lauf 41338813171).
Dies ist kein vollständiger
Windows-/macOS-Viewer-Build.
Davor:
`f2630c0e`:
Mehrfach geöffnete HTTP-Warnungen
können nur beim neuesten Dialog
einmalig zur Freigabe führen;
Toolbar-Reset erfolgt erst nach
bestätigtem Moduswechsel.
20/20 technische Prüfschritte,
194 Python-/41 native Tests,
Hauptgrid-/Beta-GUI-Negativläufe grün.
Davor:
`02af8ad4`:
OGL blockiert den alten Phoenix-/Second-
Life-Gridstatus-HTTP-Abruf auch bei
aktivierter migrierter Profileinstellung;
Optionen für Fremdgrids und Legacy-
Newsabruf sind in den lokalisierten
Einstellungen ausgeblendet,
OGL-/Classic-Modus und weitere
sichtbare Präferenztexte bereinigt.
20/20 Prüfschritte, 191 Python-
und 41 native Tests, DE-/ES-GUI-Smokes
bestanden. Davor:
`6e7c5e8c`:
Löschbestätigungen gespeicherter Konten sind
an Konto, Grid, Loginform und einmalige
Dialoggeneration gebunden; 20/20 technische
Prüfpositionen, 186 Python-/41 native Tests
und DE-/EN-GUI-Smokes grün. Sichtbare
Firestorm-Einstellungs-/Suchetitel durch
OGL/NexVerse-Bezeichnungen ersetzt.
Vorige Branding-Korrektur `224ced81`:
Support-/Registrierungs-/Downloadlinks
in 13 vorhandenen Sprachen auf NexVerse/
OGL-GitHub ausgerichtet; keine vollständige
Login-Serverantwort mehr im OGL-Fehlerlog.
20/20 separate Prüfschritte, 162 Python-
und 41 native Tests, Hauptgrid-/Beta-
GUI-Negativläufe bestanden. Vorige
Einmal-HTTP-Zustimmungs-Korrektur:
`a7490d9f`.
Voranmelde-Quit, Skalierung und Beta-Sperre sind geprüft.
**Weiter offen:** autorisierter Hauptgrid-Live-Login,
gespeicherte Passwortwiederherstellung, In-World-Shutdown
und vollständige Windows-/macOS-Viewer-Build-/GUI-Nachweise.
Alle früheren Commits/Ergebnisse dokumentiert in
[PROJECT_STATUS.md](PROJECT_STATUS.md).
Keine manuelle Zwischenabnahme; P02 erst nach dem
tatsächlich erfüllten P01-Abschluss-Gate.

Unabhängige P01-Vorarbeit (02.11.2026,
4ff7e7cb): Wenn eine Startup-Credential
zwar zum festen Grid gehört, aber nicht
als gespeichertes Konto gewählt werden kann,
verbietet OGL künftig Passwort-Fülltext und
automatische Wiederverwendung. Alle
Passwort- und Login-Zustände werden zur
manuellen Neueingabe zurückgesetzt,
auch Einwort-OpenSim-Kontonamen
korrekt angezeigt. Gültige gespeicherte
Konten und Nicht-OGL-Kanäle bleiben erhalten.
20 technische Prüfpositionen, 259 Python-
/41 native Tests, 107 Branding-/49 Grid-
Tests unter ASCII und DE-/EN-Xvfb-Login-
Smokes bestanden. Keine echten Credentials
getestet oder verändert; Live-Login und
Windows-/macOS-Vollviewer-Gates offen.
Formal P01 18/20, P02 0/20.

**P01 unabhängige Sicherheitsvorarbeit (02.11.2026,
8e9e773e):** Die alten Grid-Manager-Hilfe-
und Grid-Builder-Direktcallbacks sind selbst
abgesichert, sodass auch migrierte/alternative
Login-Skins in OGL keinen Fremd-Grid-Builder
mehr öffnen. Hilfe führt zum NexVerse-Portal.
Unverändert bleiben die zwei festen Grids und
das Verhalten anderer Viewer-Kanäle.
20 technische Prüfungen, Linux-Vollbuild
7.2.5.174, 262 Python-/41 CTests,
110 Branding-/49 Gridtests sowie DE-720p-
und EN-1080p-GUI-Smokes bestanden.
Live-Login/Windows-/macOS-Vollviewer
nicht abgenommen; formal P01 18/20,
P02 0/20. Details PROJECT_STATUS.md.

Unabhängige P01-Vorarbeit (02.11.2026, be18b791):
Retry-Zwischenspeicher für OGL-Anmeldungen
enthält nur echte, manuell eingegebene
Passwörter, niemals den angezeigten
Maskierungsplatzhalter gespeicherter
Credentials. Beim Wechsel zu einem
gespeicherten Konto wird ein altes
manuelles Retry-Passwort verworfen;
ohne echte Eingabe wird nach einem
fehlgeschlagenen Startversuch das
Passwortfeld leer und die Anmelden-
Schaltfläche deaktiviert. Gültige
gespeicherte Authenticator und andere
Viewer-Kanäle bleiben unberührt.
20 technische Prüfpunkte, Linux-
Vollbuild 7.2.5.176, 265 Python-/
41 native Tests, 113 Branding-/49 Grid-
Tests und DE-720p-/EN-1080p-GUI-Smokes
bestanden. Keine echte Credential-
Integration getestet. Formal P01
18/20, P02 0/20; restliche
End-to-End-Gates bleiben offen.

Unabhängige P01-Vorarbeit (02.11.2026, bfd5e2d9):
Benutzernamenwechsel löscht Passwort-,
Retry- und Augenknopf-Status auch bei bereits
leerem Passwortfeld. Unbekanntes Konto
synchronisiert die Passwortanzeige ebenfalls.
20 Prüfpunkte; Linux-Vollbuild 7.2.5.178,
268 Python-/41 native Tests, 116 Branding-
/49 Gridtests, DE-720p-/EN-1080p-GUI
bestanden. Keine realen Credentials getestet.
Formal P01 18/20, P02 0/20;
Live- und Plattformgates bleiben offen.

P01 Block 19/20, technische Vorprüfung
(02.11.2026, 263a224e): Main-Grid-
GET /get_grid_info HTTP 200/XML; Beta-
TLS-Hostnamenfehler 60. Authenticator-
Reuse mit mehreren @-Trennern gesperrt,
zehn native Sicherheitsvarianten.
Sechs-Plattform-CI startet bei P01-
Grid-/Login-/Startup-Codeänderungen.
20 technische Prüfungen, Linux-Vollbuild
7.2.5.180, 270 Python-/41 native Tests,
116 Branding-/51 Gridtests und DE-/EN-
GUI-Starts erfolgreich. Formales Gate
P01 19 offen: realer Live-Login und
volle Windows-/macOS-Vollviewer-GUI.
Formal P01 18/20, P02 0/20.

**Technischer Zielzustand:** OGL-Identität / sichere Anmeldung.

1. Bestehende Login-XUI, C++-Callbacks und OGL-Markenbestände inventarisieren.
2. Login-Zustand und die zwei fest eingebauten Grid-Profile abgleichen.
3. Veraltete Firestorm-Beschriftungen und sichtbares Branding erfassen.
4. Einheitliche OGL-Farb- und Typografie-Tokens in bestehenden Dateien festlegen.
5. Originale Welcome-Hintergrundfunktion beibehalten.
6. Login-Kopfbereich kompakt gestalten.
7. Benutzer- und Passwortfelder proportional, lesbar und DPI-tauglich machen.
8. Grid-Auswahl Stadt NexVerse und Stadt NexVerse Beta optisch integrieren.
9. Passwortspeicherung und Benutzerkonto-Auswahl ohne Regression erhalten.
10. Einwilligungen und Fehlermeldungen sicher und verständlich anzeigen.
11. Anzeige 'Vulkan RT bereit' aus dem Login entfernen.
12. Anzeige 'Effekt aus' aus dem Login entfernen.
13. RT-/GPU-Diagnosedaten ausschließlich in Einstellungen oder Logs belassen.
14. Schaltflächen und Tastatur-Fokuszustände vereinheitlichen.
15. DE-/EN-Texte der vorhandenen Login-Oberfläche prüfen.
16. Skalierung 720p bis 4K und High-DPI per Layouttests absichern.
17. Alte ungenutzte Login-Komponenten und Ressourcen identifizieren.
18. Nur nach Nutzungs-/Lizenzprüfung alte Login-Reste direkt entfernen.
19. Python-/native Tests, realen Linux-Viewer-Build und Grid-Login-Regression prüfen.
20. Änderungen pushen, Status/Commit dokumentieren und P02 direkt starten.

**P01 Linux-Abnahme vom 02.11.2026: 20/20.**
Der Nutzer bestätigt erfolgreichen Live-Login,
funktionierende 3D-Welt, sauberen Shutdown
und Wiederherstellung gespeicherter Konten.
Linux-Build 7.2.5.180 und 270 Python-/41
native Tests bestanden unabhängig.
Windows-/macOS-Vollviewer-Tests derzeit
mangels Testplattform nicht möglich und als
Pflicht vor jeweiligen Releases in P13
nachholen. Beta-TLS-Fehler ist P08 zugeordnet.
P02 darf unter Linux fortgesetzt werden.

### /goal P02 – OGL-Navigation, Fenster, Chat und Avatare

**Aktueller Status:** P02 aktiv, 6/20 abgeschlossen.
Block 1: Menü-/Toolbar-/Floater-/Chat-/IM-Bestände
in vorhandenen XUI-/C++-Dateien inventarisiert:
13 Menü-/11 Toolbar-Sprachlayouts, 258 EN-Floater.
Block 2 (02.11.2026, 9c10de4e): kanonische
Hauptmenü-Reihenfolge Avatar, Chat/Kontakte,
Welt, Bauen, Entdecken, Hilfe über alle 13
Sprachen. Acht ungenutzte V1-Menüs in ES
entfernt, Content-Label DA/TR lokalisiert.
RLVa, Advanced, Develop und Admin erhalten.
Linux-Vollbuild 7.2.5.186, 276 Python-/41
native Tests, drei XUI-Stagingdateien und
ES-/DA-/TR-Login-GUI bestanden.
Block 3 (02.11.2026, 4a2f34cc):
Doppelte Admin-Status-Einträge aus
elf Avatar-Menü-Overlays entfernt,
Developer-Callbacks/Shortcut erhalten;
ein EN-Toolbar-Basislayout mit zehn
Sprach-Overlays geprüft.
Linux-Vollbuild 7.2.5.188,
279 Python-/41 native Tests,
13/13 Menü-Stagingdateien und
DE-/EN-/ES-GUI-Smokes grün.
Block 4 (04.11.2026, fc097c18):
Doppeltes DevelopWorld-/leeres
Logging-Submenü in ES entfernt,
alle Funktionen weiter vorhanden;
leere DE/PT/ZH-Toolbar-Platzhalter
ohne Paralleltemplates entfernt.
Linux-Vollbuild 7.2.5.190,
282 Python-/41 native Tests,
128 Brandingtests, alle 24 XUI
und vier Paketvergleiche gültig;
vier ES/DE/PT/ZH-GUI-Smokes grün.
Block 5 (04.11.2026, 8c274734):
OGL-"Über"-Fenster zeigt im OGL-
Kanal GitHub-Support-Info, eigenes
OGL-Symbol und lokalisierten Text
auf der bestehenden Kopieren-Taste.
13 Sprachen; rechtliche Credits und
andere Viewerkanäle unverändert.
Linux-Vollbuild 7.2.5.192,
285 Python-/41 native Tests,
131 Brandingtests, 14 XUI-
Paketdateien und DE-/EN-GUI-
Smokes bestanden. Kein In-World-
About-Interaktionstest.
Block 6 (04.11.2026, 8960ecec):
Japanisches Fake-Avatar-Label und
spanischer Test-Header korrigiert;
erweiterte Chat-Zeitspalte folgt
show_time, Teleport-Trenner weiter
ohne Zeit. Avatar-Name-Cache-
Antworten werden vor Verwendung
auf richtige agent_id geprüft.
Linux-Vollbuild 7.2.5.194,
288 Python-/41 native Tests,
134 Brandingtests, 7 XML-
Header, 2 Stagingvergleiche,
JA-/ES-/DE-GUI-Smokes bestanden.
Kein In-World-Chat-Test.
Nächster Block 7/20: IM-/Gruppen-
fenster-Kopfzeilen, Status und
Navigation. Windows-Installer-
Artefakt gesondert offen.
Frühere unabhängige Vorarbeit
bereits real umgesetzt: Chat-/IM-/Gruppenfenster mit größeren
runden Avatarbildern und besseren Eingaben (`b654c299`,
20/20 technische Prüfpositionen, 167 Python/41 native Tests);
NexVerse-Portal-/Marktplatznavigation statt SL-Commerce
(`17ab85cf`, 20/20 Prüfpositionen, 170 Python/41 native Tests);
Altmenüs in 13 Sprachen entfernt, NexVerse-News/
GitHub-Issues/Release-Navigation und Skriptbibliothek-
Overlays korrigiert (`8b712092`, 20/20 Prüfschritte,
174 Python/41 native Tests);
sprachübergreifende NexVerse-Menüverlinkung und
Bereinigung der verbliebenen L$-/SL-Commerce-
Overlays (`7a3919a8`, 20/20 technische
Prüfpositionen, 178 Python-/41 native Tests,
DE-/PT-GUI-Negativläufe grün);
bestehende Toolbar/Chatbar modernisiert,
OGL-Toolbox 820×520 skalierbar und
tote Voice-/Chatbar-Overlays beseitigt
(`192bd10d`, 20/20 Prüfpositionen,
182 Python-/41 native Tests,
DE-/EN-GUI-Smoketests erfolgreich);
Such- und Einstellungsfenster ohne sichtbare
Firestorm-Titel (`6e7c5e8c`), zusätzliche
sechs Präferenzgruppen in allen vorhandenen
Sprachen rebranded (`02af8ad4`),
interne Funktionen, Rechte und rechtliche
Credits bleiben erhalten.
Unabhängige Korrektur am Toolbar-Preset:
Abbruch des Moduswechsel-Dialogs
ändert die Werkzeuge nicht mehr
(`f2630c0e`), veraltete
Bestätigungen können weder die
Presetdatei noch Toolbars ändern
(`a8951b29`, im P01-Linux-Vollbuild
geprüft). Die komplette In-World-
Visual-/Funktionsabnahme fehlt.
**Neue unabhängige P02-Vorarbeit (02.11.2026, `912a7320`):**
OGL-Browser-Präferenz öffnet nur offizielle HTTPS-NexVerse-Websites
über exakt definierte Hosts im integrierten Browser, statt
Second-Life-Links. Zehn existierende Sprach-Overlays aktualisiert,
drei erben die englische Einstellung. Linux-Vollbuild,
222 Python-/41 native Tests und isolierter EN-Login-GUI-Start
mit sauberem Exit bestanden. P01 bleibt 18/20, P02 bleibt
formal 0/20. Kein Live-/Windows-/macOS-Vollviewer-Gate behauptet.

**Weitere unabhängige P02-Vorarbeit (02.11.2026, f1552bb1):**
Legacy-Firestorm-Crashuploads unter OGL auf Linux/Windows/macOS
sowie im separaten Crash-Logger gesperrt, den irreführenden
alten Crash-Upload-Tab im OGL-Einstellungsdialog ausgeblendet.
Bestehende Benutzerpräferenzen bleiben unangetastet. Linux-
Vollbuild, 227 Python-/41 native Tests sowie isolierte DE- und
EN-GUI-Starts erfolgreich. Windows-/macOS-Vollviewer-Gates
und echter Live-Login weiterhin offen. **P01 18/20, P02 0/20
formal unverändert.** Siehe PROJECT_STATUS.md.

**Weitere unabhängige P02-Vorarbeit (02.11.2026, bf031d29):**
Firestorm-Wiki-Verweise aus 13 bestehenden Netzwerk-/
Cache-Sprachlayouts beseitigt, Chat-Befehlsdokumentation
in fünf Sprachen ohne veraltete Fremdlinks, alte
Second-Life-Kontolinkfreigabe im OGL-Kanal gesperrt.
230 Python-/41 native Tests, Linux-Vollbuild, bytegleiche
XUI-Staging-Kontrolle und DE-/EN-Xvfb-Login-Smokes grün.
Formale P01 18/20 und P02 0/20 unverändert;
Live-/Windows-/macOS-Viewer-Gates fehlen. Weitere
Details in PROJECT_STATUS.md.

Unabhängige P02-Vorarbeit (02.11.2026, ef3dc0e5):
In 20 dokumentierten Arbeitsschritten das alte
DeprecatedHelp-Untermenü aus EN und elf Overlays
entfernt, Dänisch erbt die korrigierte Basis.
OGL-/NexVerse-Hilfe, Script Library und Admin-Funktionen
erhalten; Advanced-Webbrowser-Test nutzt NexVerse.
Windows-cp1252-CI-Testlesefehler mit explizitem UTF-8
korrigiert und unter ASCII-Systemkodierung geprüft.
Linux-Vollbuild, 232 Python-/41 native Tests, 13/13
XUI-Staging und DE-/EN-GUI-Smokes grün. Kein vollständiger
Windows-/macOS-Viewer- oder Live-Login-Nachweis.
Formal P01 18/20, P02 0/20. Details im Status.

Unabhängige P02-Vorarbeit (02.11.2026, 6b6779fc):
System-/Region-Chat im aktiven FS-Chatverlauf,
Nearby-Chat-Toasts und beide Benachrichtigungsansichten
verwenden das vorhandene OGL-Logo statt des
Second-Life-Systemicons. Radar-Avatare, RLVa-
Namensfilter und Nicht-OGL-Fallbacks bleiben
im Quellpfad erhalten. Demo-Name/-Uhrzeit
im Chatkopf entfernt, fünf Regressionstests ergänzt.
20 technische Prüfpunkte, 237 Python-/41 native
Tests, kompletter Linux-Build, Ressourcengleichheit
im Build und DE-/EN-GUI-Smokes bestanden.
Live-/Windows-/macOS-Vollviewer-Gates unverändert offen.
Formal P01 18/20, P02 0/20; Projektstatus maßgeblich.

**Unabhängige P02-Vorarbeit (02.11.2026, bde46c18):**
Aktiver FSChatHistoryHeader behält eine feste,
ellipsierte Zeitangabe rechts und bietet den ganzen
Wert über den Tooltip; lange History-Zeitstempel
können die Avatar-/Namens-Layoutgruppe nicht mehr
dynamisch verschieben. Demo-Zeitwerte aus DE, JA,
PT und TR entfernt; drei Regressionstests ergänzt.
20 technische Prüfpunkte dokumentiert, 240 Python-
/41 native Tests, Linux-Vollbuild, sieben bytegleiche
XUI-Stagingdateien und isolierte DE-/EN-GUI-Smokes
bestanden. Reale In-World- und vollständige
Windows-/macOS-Vollviewer-Gates bleiben offen.
**Formal P01 18/20, P02 0/20.**
Details in PROJECT_STATUS.md.

**Unabhängige P02-Vorarbeit (02.11.2026, cfa671e4):**
OGL-spezifische Abschaltung der alten Firestorm-
Supportgruppen-Präfixe mit OS-/Viewer-/Skin-/
RLVa-Metadaten sowie der alten Schalter,
Beitritts-Dialoge und Sysinfo-IM-Sendefunktion,
einschließlich verzögerter Bestätigungen.
Nicht-OGL-Verhalten und normale IM-Sendefunktion
bleiben erhalten. 20 dokumentierte technische
Prüfpositionen, Linux-Vollbuild, 245 Python-/
41 native Tests, 93 Branding-Tests (auch unter
ASCII), DE-/EN-Login-GUI-Smokes erfolgreich.
Keine In-World- oder vollständigen Windows-/macOS-
Viewer-Gates behaupten. Formal P01 18/20,
P02 0/20. Details in PROJECT_STATUS.md.

Unabhängige P02-Vorarbeit (02.11.2026, 8deebedc):
Negative Unread-Zähler in Nearby-Chat (normaler/
stummgeschalteter History-Cache) und IM werden vor
Cache/Anzeige auf null begrenzt. Präferenzschalter,
Sprachlayouts und der bereits korrekt initial
unsichtbare Chat-Hinweis bleiben unverändert.
20 technische Prüfpunkte, 250 Python-/41 native
Tests, 98 Branding-/Chat-Tests, Linux-Vollbuild
und DE-/EN-Xvfb-GUI-Smokes erfolgreich.
Beta-TLS-Zertifikatsabweichung am 02.11. erneut
bestätigt, Anmeldung weiterhin gesperrt.
Formal P01 18/20, P02 0/20. Siehe Status.

Unabhängige P02-Vorarbeit (02.11.2026, a462330e):
Die Chat-Header-Namensspalte wird nicht mehr
zusätzlich zur bestehenden XUI-Zeitspalte
dynamisch verkleinert. Alten Draw-Sonderweg,
unbenutzte Layout-Zustände und sechs
Breitenupdate-Aufrufe entfernt; Tooltips,
RLVa-Anonymisierung, runde Avatare und
Teleport-Trennzeilen erhalten. 20 technische
Prüfschritte, 253 Python-/41 native Tests,
101 Branding-/Chatlayout-Tests, voller
Linux-Build sowie DE-/EN-Login-GUI-Smokes
erfolgreich. Authentifizierter In-World-
und vollständiger Windows-/macOS-Viewer-
Nachweis weiter offen. Formal P01 18/20,
P02 0/20; PROJECT_STATUS.md ist maßgeblich.

Unabhängige P02-Vorarbeit (02.11.2026, 72c8beed):
In neun bestehenden Sprach-strings.xml die
zentralen Vieweridentitäten Firestorm/Second
Life auf OGL Viewer umgestellt und feste
Grid-/System-Namen zu [CURRENT_GRID]
normalisiert. Vier weitere Sprachen übernehmen
die korrigierten englischen Basiswerte.
Keine neuen Sprachdateien und keine Änderung
an rechtlichen Credits. Drei Regressionstests
ergänzt. 20 technische Prüfpunkte; Linux-
Vollbuild 7.2.5.170, 256 Python-/41 native
Tests, 104 Branding-Tests, 13 bytegleiche
Staging-Sprachen und vier GUI-Voranmelde-
Starts ES/RU/TR/ZH bestanden. Echte
In-World-/Windows-/macOS-Gates bleiben
offen. Formal P01 18/20, P02 0/20.

**Technischer Zielzustand:** Einheitliche sichtbare Viewer-Oberfläche.

1. Vorhandene Navigation, Menüs und Floater im Nutzungsfluss inventarisieren.
2. Einheitliche OGL-Navigationshierarchie definieren.
3. Bestehende Hauptnavigation ohne parallele Template-Dateien umbauen.
4. Untermenüs und Toolbar-Struktur konsistent gestalten.
5. Fenster-Titel, Icons und Schaltflächen zu OGL/NexVerse vereinheitlichen.
6. Unnötiges sichtbares Firestorm-Branding entfernen.
7. Allgemeine Fenstergrößen- und DPI-Regeln zusammenführen.
8. Lokalen Chat mit modernem Nachrichtendesign abgleichen.
9. IM-Chat mit demselben Nachrichtenschema abgleichen.
10. Gruppenchat mit demselben Nachrichtenschema abgleichen.
11. Runde echte Avatarporträts statt quadratischer Vorschaubilder verwenden.
12. Avatarname und Uhrzeit über dem Nachrichtentext ausgeben.
13. Nachrichtenbereich und Eingabefeld responsive machen.
14. Kontaktliste und Sitzungsanzeige funktional konsolidieren.
15. Avatar-/Profil-Privatsphäre und OpenSim-Rechte wahren.
16. Voice-/Chat-Funktionen und Kontextmenüs unverändert bedienbar halten.
17. DE-/EN-Text- und Tastaturbedienung prüfen.
18. Verwaiste Menüs, XUI-Elemente und doppelte Referenzen bereinigen.
19. Vollviewer-Tests, Vergleichsansichten und Performance-Regression prüfen.
20. Commit, Status und Ergebnisse melden; P03 ohne Nutzerabnahme starten.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

Historische Vermerke auf P01 18/20 weiter
oben beziehen sich auf Vorarbeiten vor der
Linux-Funktionsbestätigung. Windows/macOS
bleiben bis zum vollständigen OS-Release-Gate
unfreigegeben.

Windows-Abnahme-Vorbereitung (02.11.2026):
Im vorhandenen OGL-GitHub-Workflow wurde ein
manuell per markiertem Push [windows-installer]
startbarer Windows-2022/x64-Vollviewer-
OpenSim-Build mit unsigniertem NSIS-Setup
als zeitlich begrenztem Artefakt vorbereitet.
Die sechs nativen CI-Moduljobs ersetzen
keinen installierbaren Viewer. Windows-
Abnahme erfolgt erst nach erfolgreichem
Installer-Build und realem Login/GUI-Test;
spätere Plattform-Release-Gates in P13
bleiben bestehen. P02 läuft unter Linux weiter.

### /goal P03 – Theme-System und sichere Designinstallation

**Status:** geplant; 0/20 operative Blöcke abgeschlossen.
**Technischer Zielzustand:** Optionale Designs bei OGL-Standard.

1. Vorhandenes OGL-Designpanel und Theme-CLI inventarisieren.
2. OGL/NexVerse als sichtbaren Standard statt Firestorm-Altstil definieren.
3. Den reversiblen kompatiblen Fallback technisch erhalten.
4. Theme-Manifest und Versionsvertrag prüfen.
5. Farb- und Textur-Tokens zentralisieren.
6. Layout- und Template-Zuordnung ohne Duplikate entwerfen.
7. Native Dateiauswahl für .ogltheme in bestehende UI einbauen.
8. ZIP-Pfadtraversal-, Symlink-, Größen- und Formatprüfung absichern.
9. Paketauthentizität und erforderliche Berechtigungen festlegen.
10. Atomische Installation im Benutzerprofil sicherstellen.
11. Installierte Pakete im Viewer auflisten.
12. Pakete vor Aktivierung visuell vorschauen.
13. Theme-Änderungen und Neustarterfordernis verständlich machen.
14. Theme aktivieren und OGL-Standard wiederherstellen.
15. Theme-Pakete gefahrlos deaktivieren/entfernen.
16. Sprache, HiDPI und Chat-/Menü-Kompatibilität prüfen.
17. Manipulierte Themes und Updatefehler per Testpaketen abweisen.
18. Nicht verwendete Skin-Overrides und Theme-Duplikate bereinigen.
19. Linux-Build sowie portable Theme-Sicherheitstests ausführen.
20. GitHub/Status aktualisieren und P04 direkt beginnen.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

### /goal P04 – Renderer-Abstraktion und OpenGL-Fallback

**Status:** geplant; 0/20 operative Blöcke abgeschlossen.
**Technischer Zielzustand:** Saubere Backend-Auswahl und Baseline.

1. OpenGL-Renderpfad und Vulkan-Diagnosemodule vollständig inventarisieren.
2. Gemeinsame Render-Backend-Funktionen und Grenzen spezifizieren.
3. Bestehende Schnittstellen erweitern statt doppelte Renderer-Dateien erstellen.
4. Renderer-Lebenszyklus und Thread-/Ressourcenbesitz definieren.
5. Grafikgeräte und API-Funktionsumfang plattformneutral ermitteln.
6. GPU-Speicherbudget, Treiber und Fehlerzustände erfassen.
7. OpenGL als Standard/Kompatibilitätsmodus festlegen.
8. Neue Engine-Auswahl in vorhandene Grafikeinstellungen aufnehmen.
9. Vulkan nur bei geeigneter CPU/GPU/Treiber-Kombination anbieten.
10. Gewählte und letzte erfolgreiche Engine getrennt sichern.
11. Enginewechsel mit kontrolliertem Neustart vorsehen.
12. Bei Vulkan-Startfehler automatisch auf OpenGL zurückfallen.
13. OpenGL-Einstellungswerte und Nutzerdaten erhalten.
14. Logging für Backend-Auswahl ohne störende Login-Debugtexte einbauen.
15. Headless-/Mock-Tests für Rendererentscheidung ergänzen.
16. Alte Treiber, Intel/AMD/NVIDIA und ARM-Kompatibilität berücksichtigen.
17. Renderer-Abstraktion auf Linux mit echtem Viewer kompilieren.
18. Windows-/macOS-Modul-CI und Fehlerfälle prüfen.
19. Ungebrauchte Grafikdiagnose-Hooks nach Abhängigkeitsprüfung bereinigen.
20. Änderungen veröffentlichen und P05 direkt starten.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

### /goal P05 – Vulkan als vollständige auswählbare 3D-Engine

**Status:** geplant; 0/20 operative Blöcke abgeschlossen.
**Technischer Zielzustand:** Welt-Rendering statt Diagnose.

1. Vulkan-Instance/Device/Queue-Initialisierung mit Schutzprüfungen integrieren.
2. Window-Surface und Swapchain für Desktopbetrieb anbinden.
3. Frame-Lifecycle, Synchronisation und Resize kontrollieren.
4. Vertex-/Indexbuffer-Verwaltung mit Scene-ABI verknüpfen.
5. Terrain- und Landoberflächen im Vulkan-Backend darstellen.
6. Mesh- und Objektgeometrie portieren.
7. Textur-Uploads und Caching anbinden.
8. Alpha/Blend-/Transparenzregeln übernehmen.
9. Material-/PBR-Shaderpfade portieren.
10. Licht, Sonne, Himmel und Wetter abbilden.
11. Avatar-/Attachment-Geometrie übernehmen.
12. Partikel- und Wasser-Darstellung integrieren.
13. HUDs, Labels und Text-Overlays rendern.
14. UI-Compositing und Eingaben erhalten.
15. Kamera, Auswahl, Teleport und Regionwechsel testen.
16. GPU-Memory-/Descriptor-/Pipelinefehler abfangen.
17. Treiber-/OS-Ausfälle kontrolliert auf OpenGL zurückführen.
18. Vergleichsbilder Vulkan/OpenGL für reale OpenSim-Szenen prüfen.
19. Linux-Viewer-Build, portable Tests, FPS/VRAM-Messungen durchführen.
20. Stabilen Stand veröffentlichen, Status melden, P06 starten.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

### /goal P06 – Vulkan-Raytracing und Grafikqualität

**Status:** geplant; 0/20 operative Blöcke abgeschlossen.
**Technischer Zielzustand:** Optionale Effekte auf kompatibler GPU.

1. Raytracing-Prototyp und bisherige GPU-Diagnosen inventarisieren.
2. Ray-Query/RT-Pipeline-Fähigkeiten zur Laufzeit erkennen.
3. Keine Markenbindung an NVIDIA/AMD/Intel erzwingen.
4. RT nur innerhalb einer funktionierenden Vulkan-Engine aktivieren.
5. Nicht-RT-fähige GPUs mit normalem Vulkan rendern lassen.
6. BLAS/TLAS-Szenenaktualisierung und Lebenszyklen absichern.
7. Statische/dynamische Mesh-Änderungen berücksichtigen.
8. Raytraced-Schatten aus realer Weltgeometrie erzeugen.
9. Sonnen-/Himmelslicht in Beleuchtung einbinden.
10. Reflexionen nur bei tragbarer GPU-Last ergänzen.
11. Temporale Stabilität/Anti-Aliasing und Denoser prüfen.
12. Grafikeffekte in die bestehenden Einstellungen integrieren.
13. Ungeeignete RT-Optionen automatisch deaktivieren.
14. GPU-Last-/VRAM-/FPS-Profile dokumentieren.
15. OpenGL-Welt und Vulkan ohne RT unverändert nutzbar lassen.
16. RTX-/Diagnosetexte aus dem Login fernhalten.
17. Renderfehler, Teleport, Regionwechsel und GPU-Reset prüfen.
18. Veraltete Debug-Proben nur nach Regressionstest bereinigen.
19. Linux- und Cross-OS-Tests sowie echte Szenenvergleiche durchführen.
20. Ergebnisse veröffentlichen und P07 starten.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

### /goal P07 – CPU/GPU-Leistung, Textur-, Mesh- und Avatarcache

**Status:** geplant; 0/20 operative Blöcke abgeschlossen.
**Technischer Zielzustand:** Nachweisbare Performanceverbesserung.

1. CPU- und GPU-Metriken und vorhandene Cachepfade inventarisieren.
2. Reproduzierbare OpenSim-Testszenen und Messwerte festlegen.
3. Skalaren CPU-Fallback absichern.
4. SSE2-Pfad für x86-64 prüfen.
5. AVX2-Pfad nur bei CPU- und OS-Unterstützung aktivieren.
6. ARM64-NEON-Pfad separat prüfen.
7. CPU-Benchmarks gegen systemoptimierte Vergleichsfunktionen messen.
8. Textur-Netzwerk-/Decode-/Cache-Latenzen instrumentieren.
9. Mesh-Laden und Geometry-Cache messen.
10. Avatar-Baking/Attachments und Appearance-Pipeline messen.
11. Unnötige Cache-Neuberechnungen identifizieren.
12. Cache-Prefetch und Wiederverwendung sicher optimieren.
13. CPU-/GPU-Upload-Stau und Frame-Stottern messen.
14. RAM-/VRAM-Budgets und Cache-Obergrenzen definieren.
15. Fehlerhafte Texturen/Meshes und Offline-Fälle berücksichtigen.
16. Alte Systeme mit OpenGL und begrenztem Arbeitsspeicher testen.
17. Keine nicht gemessenen FPS-/Ladezeitversprechen machen.
18. Unwirksame/duplizierte Optimierungspfade entfernen.
19. Regressionen und Performance-Vergleichswerte dokumentieren.
20. GitHub/Status aktualisieren und P08 starten.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

### /goal P08 – Beta-Protokoll und getrennte NexVerse-Anmeldung

**Status:** geplant; 0/20 operative Blöcke abgeschlossen.
**Technischer Zielzustand:** Sichere zweite Verbindung.

1. Bestehenden öffentlichen OpenSim-Login auf Regression prüfen.
2. Beta-TLS-Zertifikat und Hostnamen korrekt verifizieren.
3. Beta-Login-API/Protokollvertrag vom Backend feststellen.
4. Auth-Adapter ohne fremde Passwortweiterleitung abgrenzen.
5. Manuelle und automatische Login-Gates einheitlich machen.
6. Credentials und Tokens je Grid getrennt speichern.
7. Beta-Session-Aufbau und Ablauf implementieren.
8. Timeout, Retry, Reconnect und Logout absichern.
9. Netzwerk-/EventQueue-/Capabilities-Unterschiede erfassen.
10. Avatar-/Benutzeridentität pro Grid zuordnen.
11. Regionenverbindung und Standortdaten adaptieren.
12. Teleport/Regionswechsel nur bei bestätigter API anbieten.
13. Chat-/IM-Fähigkeiten anhand des Beta-Protokolls anbinden.
14. Inventar- und Medienoperationen nach Capability trennen.
15. Protokollfehler verständlich und ohne Geheimnislecks darstellen.
16. Unbekannte Grids/Redirects/TLS-Bypass sperren.
17. Beta/öffentliches Grid parallel mit unabhängigen Tests prüfen.
18. Ungenutzte Altadapter und doppelte Netzwerkpfade bereinigen.
19. Build/CI und servergestützte Integrationstests durchführen.
20. Freigabe melden und P09 starten.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

### /goal P09 – Abgesicherter Plugin-Manager

**Status:** geplant; 0/20 operative Blöcke abgeschlossen.
**Technischer Zielzustand:** Versionierte Erweiterungen ohne unsichere Fremd-DLL.

1. Vorhandene Modul-ABI und Registry inventarisieren.
2. Plugin-API-Versionierung und Kompatibilitätsregeln festlegen.
3. Funktions-/Berechtigungsmanifest spezifizieren.
4. Plugin-Paketformat und Plattform-Asset-Varianten prüfen.
5. Authentizität und Signaturschlüsselmodell entwickeln.
6. Rechtevergabe und Widerruf in UI abbilden.
7. Plugins ohne Root-/Adminrechte installieren.
8. Codeausführung nach Möglichkeit in getrennten Prozessen isolieren.
9. IPC- und Capability-Grenzen festlegen.
10. Plugin-Lebenszyklus Start/Stop/Update absichern.
11. API-Fehler und Hänger ohne Viewer-Absturz behandeln.
12. Plugin-Konfiguration getrennt vom Viewer bewahren.
13. Plugin-Updates und Rückrollbarkeit unterstützen.
14. Sicheres Entfernen und Datenbereinigung implementieren.
15. UI-Anzeige installierter Plugins und Berechtigungen erstellen.
16. Linux-/Windows-/macOS-Abweichungen abstrahieren.
17. Malware-/Pfad-/Signatur-Negativtests durchführen.
18. Nicht mehr gebrauchte Modulprototypen nach Tests bereinigen.
19. Vollviewer- und CI-Tests mit erlaubten Testplugins ausführen.
20. Commit/Status aktualisieren und P10 beginnen.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

### /goal P10 – Automatisierte signierte GitHub-Releases

**Status:** geplant; 0/20 operative Blöcke abgeschlossen.
**Technischer Zielzustand:** Nachweisbar verfügbare Installer.

1. GitHub-Actions-, Release- und Paket-Skripte inventarisieren.
2. Versionsnummer/Tag/Stable-/Beta-Kanal festlegen.
3. Release-Gates durch Build-, Unit- und Integrationstests definieren.
4. Linux-x86-64-Viewer-Paket reproduzierbar erstellen.
5. Linux-ARM64-Viewer-Paket kompatibel bauen.
6. Windows-x64-Paket und Starttest integrieren.
7. Windows-ARM64-Paket und Starttest integrieren.
8. macOS-Intel-Paket und Starttest integrieren.
9. macOS-Apple-Silicon-Paket und Starttest integrieren.
10. Code-Signing/Notarisierung und Secret-Verwaltung absichern.
11. Build-Metadaten und eindeutige Dateinamen vereinheitlichen.
12. SHA-256/Signatur/Release-Manifest erzeugen.
13. Nightly/Beta gegen stabile Releases abgrenzen.
14. Automatisches GitHub-Release nur nach grünen Freigabegates.
15. Fehlerhafte Artefakte dürfen nicht als Stable veröffentlicht werden.
16. README und offizielle Downloadadressen pro OS/CPU verifizieren.
17. Release-API und Upload-Rate-Limits berücksichtigen.
18. Veraltete doppelte CI-/Paketierungsdateien bereinigen.
19. Downloads auf mehreren Plattformen installieren/starten.
20. Release und Status veröffentlichen; P11 starten.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

### /goal P11 – Clientseitiger Pflicht-Updater und Update-UI

**Status:** geplant; 0/20 operative Blöcke abgeschlossen.
**Technischer Zielzustand:** Vor Login aktuelle freigegebene Version.

1. Vorhandenen Velopack-Code und OGL-Login-Gates inventarisieren.
2. Einheitliche Release-/Mindestversions-Schnittstelle implementieren.
3. Vertrauenswürdiges signiertes Update-Manifest abrufen.
4. HTTPS/Zertifikat/Timeout/Rate-Limit sicher prüfen.
5. OS-/Architektur-/Channel-kompatibles Release auswählen.
6. Versionsnummern numerisch und downgrade-sicher vergleichen.
7. Pflicht- und optionale Updates unterscheiden.
8. Prüfung vor allen manuellen und Auto-Login-Pfaden einbauen.
9. Pflichtupdate-Dialog mit Versions- und Änderungsinformationen.
10. Abbrechen sperrt Login statt nur Dialog zu schließen.
11. Downloadfortschritt, Wiederholung und Fehler anzeigen.
12. Hashes/Signaturen der Pakete vor Installation prüfen.
13. Installation mit Erhalt von Profil/Cache/Themes/Plugins durchführen.
14. Windows-Updater gegen signierte Pakete testen.
15. macOS-Updater gegen signierte/notarisierte Pakete testen.
16. Linux-Updatepfad mit Benutzerrechten implementieren.
17. Neustart und Wiederaufnahme der Anmeldung prüfen.
18. Offline- und fehlerhafte Manifestfälle fail-closed behandeln.
19. Umgehungsversuche der Client-Updateprüfung testen.
20. CI/GitHub/Status aktualisieren und P12 beginnen.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

### /goal P12 – Serverseitige Mindestversion für beide Grids

**Status:** geplant; 0/20 operative Blöcke abgeschlossen.
**Technischer Zielzustand:** Sperre darf nicht clientseitig umgangen werden.

1. Öffentlichen OpenSim-Login-Service und Beta-Backend-Zugriffe festlegen.
2. Getrennte Minimum-Version-Policies je Grid spezifizieren.
3. Update-required-Fehlervertrag für beide Protokolle definieren.
4. Server überprüft Version vor Ausgabe einer Welt-Session.
5. Versionierung des Clients nicht blind als unverfälschbar behandeln.
6. Vertrauens-/Attestierungsgrenzen und Bypassrisiken dokumentieren.
7. Stabile und Beta-Mindestversion getrennt konfigurieren.
8. Serverantwort enthält nur vertrauenswürdige Updateinformationen.
9. Öffentlichen OpenSim-Gateway-Adapter implementieren.
10. Beta-Authentifizierungsdienst nach demselben Vertrag anbinden.
11. Timeouts und Update-Service-Ausfälle absichern.
12. Alte Viewer-Version ohne aktive Welt-Session zurückweisen.
13. Aktuelle freigegebene Version durchlassen.
14. Manipulierte Versionsheader und unbekannte Clients testen.
15. Session-Revalidierung für bereits eingeloggte Clients definieren.
16. Server-Logs und datensparsame Prüfmetriken ergänzen.
17. Client und Server mit einer vollständigen Testmatrix verbinden.
18. Alte doppelte Gate-/Policy-Verzweigungen bereinigen.
19. Zugriff und Zuständigkeit anderer Backend-Repositories dokumentieren.
20. Serverseitigen Teststand melden und P13 beginnen.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

### /goal P13 – Portal-Releases, End-to-End-QA und Dauerbereinigung

**Status:** geplant; 0/20 operative Blöcke abgeschlossen.
**Technischer Zielzustand:** Portal, Viewer und Server konsistent.

1. Portal-Repository/Deployment-/Berechtigungen verifizieren.
2. Offizielle GitHub-Releases-API als einzige Releasequelle anbinden.
3. Backend-Cache und GitHub-API-Rate-Limit-Behandlung implementieren.
4. Stable- und Beta-Releases korrekt trennen.
5. OS/CPU-Assets im Portal passend anzeigen.
6. Download-URLs und Releasesignaturen serverseitig überprüfen.
7. Releaseversion und Änderungsinformationen verständlich darstellen.
8. Portal-API-Fallback bei GitHub-Ausfall kontrollieren.
9. Update-Link vom Viewer zum passenden Release prüfen.
10. Öffentlichen OpenSim-Login end-to-end testen.
11. Beta-Login mit gültigem Protokoll end-to-end testen.
12. Pflichtupdate: veralteter Viewer verweigert und aktualisiert.
13. Pflichtupdate: aktueller Viewer wieder zugelassen.
14. OpenGL-Standard auf älterer Hardware prüfen.
15. Vulkan-Engine und optionales RT unabhängig prüfen.
16. Linux x64/ARM64 installieren und echten Start testen.
17. Windows x64/ARM64 installieren und echten Start testen.
18. macOS Intel/Apple Silicon installieren und echten Start testen.
19. Altbestände/duplizierte Templates/CI-/Assets mit Tests bereinigen.
20. Release-/Portal-/Projektstatus veröffentlichen, nächsten Zyklus planen.

**Abschluss-Gate:** Alle 20 Punkte implementiert oder als
bereits nachweislich vorhanden geprüft, relevante Tests
und Viewer-/Plattform-Builds erfolgreich, GitHub-Push
sowie Status aktualisiert; keine Benutzerabnahme erforderlich.

### Fortschrittsformat in PROJECT_STATUS.md

Für jede Phase `P01` bis `P13` führen:
`offen | aktiv | blockiert | fertig`, `x/20`, letzten
geprüften Commit, Testnachweise, offene Abhängigkeiten und
`Nächster Schritt: Pxx Block yy`. Technisch vorhandene
Alt-Funktionen können nach Verifikation direkt als erledigte
Blöcke zählen; bloßes Vorhandensein eines Mockups genügt nicht.

**Bei /goal-Abschlussmeldungen keine Rückfrage „weiter?“,
keine Pausen zur Freigabe und keine manuelle Phasenabnahme.**
Nur nicht auflösbare technische oder rechtliche Blocker
oder ausdrücklich notwendige Zugriffsrechte nennen.

## Fachliche Referenzphasen A–I (keine aktive Ausführungsreihenfolge)

Die folgenden älteren Phasenkapitel liefern Kontext und technische
Teilspezifikationen; operativ gilt ausschließlich P01–P13 oben.
Bereits implementierte Teile werden in den zugehörigen P-Phasen
verifiziert und wiederverwendet.

### Referenz Phase A – zwei feste NexVerse-Login-Profile (20 Blöcke)

1. Erledigt: öffentliches NexVerse-OpenSim-Grid beibehalten.
2. Erledigt: Beta-Schlüssel und HTTPS-Website fest definieren.
3. Erledigt: beide als feste Systemgrids registrieren.
4. Erledigt: bestehende Welcome-Seite beibehalten.
5. Erledigt: alle anderen Grids aus der OGL-Auswahl entfernen.
6. Erledigt: externe Grid-Overrides und gefährliche Re-Fetches sperren.
7. Erledigt: Dropdown im echten Firestorm-Login freischalten.
8. Erledigt: Dropdown ungeachtet älterer ForceShowGrid-Werte anzeigen.
9. Erledigt: Beta-Auswahl nach Viewer-Neustart wiederherstellen.
10. Erledigt: unbekannte alte Grid-Auswahl aufs Hauptgrid zurücksetzen.
11. Erledigt: Freitext-Grid-Adressen zurückweisen.
12. Erledigt: HTTP-Ausnahme nur fürs öffentliche Grid belassen.
13. Erledigt: automatische Beta-Authentifizierung vor Netzwerkaufruf sperren.
14. Erledigt: manuellen Beta-Login vor Passwortauswertung stoppen.
15. Erledigt: lokalisierten Hinweis in Deutsch und Englisch anzeigen.
16. Erledigt: Sicherheits- und Grid-Regressionstests ergänzen.
17. Erledigt: vollständigen Linux-Firestorm-Build erzeugen.
18. Erledigt: 119 Python- und 41 native Tests bestanden.
19. Erledigt: sechs plattformübergreifende CPU-/Modul-CI-Jobs bestanden.
20. Ausstehend: visuell beide Grid-Einträge und das Hauptgrid prüfen.

**Sicherheitsgrenze:** Das TLS-Zertifikat für
https://beta.stadt-nexverse.de passt am 01.11.2026 nicht zur Domain.
Ein kompatibles Beta-Anmeldeprotokoll wurde nicht verifiziert.
Die Beta ist daher ausschließlich auswählbar; Login bleibt blockiert.

### Referenz Phase B – separates Beta-Protokoll (je 20 Blöcke)

Zertifikat und dokumentierten Auth-Vertrag verifizieren; modularen
OpenSim-/Beta-Loginadapter implementieren; Zugangsdaten,
Sessions und Tokens sauber isolieren. Region, Avatar, Inventar,
Chat, Teleport und Fehlerverhalten erst nach Protokollabnahme
über adapterbasierte APIs anschließen.

### Referenz Phase C – Designs, Templates und Chat

Bestehenden OGL-Designs-Reiter ausbauen, native sichere ZIP-Installation,
Live-Vorschau und Rücksetzung auf das **OGL-Standarddesign** ermöglichen.
Das originale Firestorm-Design darf nicht mehr als reguläre
Nutzeroberfläche erscheinen; technische/urheberrechtlich nötige
Referenzen bleiben erhalten. Chat-, IM- und Gruppenfenster
einheitlich mit rundem Avatarbild, Namen, Uhrzeit und Text darstellen.
Multi-DPI und kleine/große Auflösungen testbar gestalten.
Detaillierte, auf 20 Blöcke aufteilbare UI-Abnahmen: Phase H.

### Referenz Phase D – abgesicherter Plugin-Manager

Manifest, Capabilities, Signaturen, Sandbox, API-Version, Benutzer-
freigabe, Update und Widerruf. Keine ungeprüften DLL/SO/DYLIB laden.

### Referenz Phase E – CPU/GPU, Cache, Grafik und Releases

Portable CPU-Wege scalar/SSE2/AVX2/ARM64-NEON, Grafikfähigkeiten
dynamisch erkennen und alte Hardware über OpenGL absichern.
Textur-, Mesh- und Avatar-Ladezeiten messen und verbessern.
Vulkan-RT-Tests bleiben optional; **Vulkan als eigene,
auswählbare Grafik-Engine** wird gesondert in Phase G entwickelt.
Linux-, Windows- und macOS-Builds einschließlich x64 und ARM64
separat bauen, signieren und testen.

### Referenz Phase F – verpflichtender Viewer-Updater

Die verbindliche Mindestversion muss vor dem Login geprüft werden.
Der Viewer bietet Pflichtupdates an, der NexVerse-Server lehnt
nicht mehr zugelassene Versionen ab. Das System muss Linux,
Windows und macOS samt x64-/ARM64-Paketen unterstützen.
20 Entwicklungsblöcke und Abnahmekriterien stehen in
GOAL_MANDATORY_VIEWER_UPDATER.md.

**Status: geplant, nicht implementiert.**

### Referenz Phase G – auswählbare Grafik-Engines OpenGL und Vulkan (20 Blöcke)

**Status: geplant, kein produktiver Vulkan-Welt-Renderer vorhanden.**

1. Existierenden OpenGL-Renderer und Vulkan-RT-Prototyp getrennt inventarisieren.
2. Rendering-Backend-Schnittstelle und Funktionsgrenzen definieren.
3. Bestehende Renderer-Dateien direkt erweitern, keine Ersatzdateien erstellen.
4. Geräte-/Treiber-/Vulkan-API-Version plattformübergreifend erkennen.
5. Unterstützte GPU-Features und GPU-Speichergrenzen klassifizieren.
6. Engine-Auswahl in die vorhandenen Grafikeinstellungen integrieren.
7. OpenGL als sichere Standardeinstellung und Rückfalloption belassen.
8. Letzte funktionierende Engine getrennt vom Wunschwert speichern.
9. Vulkan-Initialisierung mit Fehlerbehandlung und Rückfall absichern.
10. Swapchain, Fenstergrößenwechsel, DPI und Vollbild portieren.
11. Vulkan-Ressourcenverwaltung und Speicherbudget absichern.
12. Bestehende Terrain- und Objekt-Geometriepfade portieren.
13. Texture/Material/Alpha/PBR-Verarbeitung auf Vulkan portieren.
14. Shader-/Pipelineverwaltung, Licht und Schatten integrieren.
15. Avatar-, Partikel-, Mesh- und HUD-Darstellung angleichen.
16. Kamera-, Auswahl-, UI-Overlay- und Postprocessingpfade angleichen.
17. Optionales Raytracing ausschließlich bei passenden RT-Funktionen.
18. Enginewechsel/Neustart/Absturz und fehlende Treiber testen.
19. OpenGL-Vulkan-Bildvergleich und FPS/VRAM/Startzeit auf OS/CPU/GPU testen.
20. Altcodebereinigung, vollständige Viewer-Builds, manuelle Abnahme, GitHub.

**Abnahme:** Beide Engines sind im selben Viewer auswählbar;
OpenGL funktioniert auf älterer Hardware. Ungeeignete GPUs können
Vulkan nicht aktivieren bzw. fallen auf OpenGL zurück. Eine
RT-fähige GPU ist keine Voraussetzung für allgemeines Vulkan.
Keine Behauptung eines vollständigen Vulkan-Renderers allein
auf Grundlage der bisherigen Raytracing-/Diagnosemodule.

### Referenz Phase H – OGL/NexVerse-Branding, Navigation und Login (20 Blöcke)

**Status: geplant; vorhandene Chat-/Login-Arbeiten sind Teilstände.**

1. Alle sichtbaren Firestorm-Marken, Icons, Fenster und Texte inventarisieren.
2. Einheitlichen OGL/NexVerse-UI-Styleguide für Nutzeroberfläche festlegen.
3. Bestehende Navigationsstruktur in denselben XUI-Dateien modernisieren.
4. Einheitliche Haupt-/Sekundärnavigation und Fensterhierarchie aufbauen.
5. Login-Bereich verkleinern und Lesbarkeit verbessern.
6. Grid-Auswahl beider NexVerse-Ziele erhalten und sauber gestalten.
7. Benutzername/Passwort/Speicherung/Einwilligungen korrekt einordnen.
8. „Vulkan RT bereit“ aus dem Login vollständig entfernen.
9. „Effekt aus“ aus dem Login vollständig entfernen.
10. RT-/GPU-Diagnosen ausschließlich in Grafikeinstellungen/Logs verlegen.
11. Welcome-Hintergrund beibehalten, ohne ihn mit UI zu überdecken.
12. OGL-Logo, Titel, Icons, Ladebildschirme und Produktnamen vereinheitlichen.
13. Chat und IM/Gruppe mit runden Avataren, Namen, Uhrzeit und Text angleichen.
14. Responsives Layout für kleine/große Bildschirme und HiDPI.
15. Tastatur, Fokus, Screenreader und DE/EN-Übersetzungen prüfen.
16. Skin/Template-Erweiterungen nur über vorhandene Dateien umsetzen.
17. Original-Firestorm-UI im sichtbaren Ablauf vollständig zurückbauen.
18. Lizenz-/Copyright-/Quellhinweise intern weiterhin korrekt erhalten.
19. Login, Gridwechsel, Einstellungen, Weltansicht und Fenster testen.
20. Verwaiste UI-Dateipfade sicher bereinigen, Plattformtests und Release.

**Abnahme:** Keine sichtbaren Firestorm-Bedienelemente oder
Firestorm-Marken in der regulären Viewer-Oberfläche. Copyright-
und Lizenzpflichten werden trotzdem eingehalten; OpenGL-Fallback
und beide festen Grid-Profile bleiben funktionsfähig.

### Referenz Phase I – GitHub-Releases, Portal-Anbindung, Quellbaumbereinigung (20 Blöcke)

**Status: geplant; Phase F bleibt für die Login-Sperre zuständig.**

1. Bestehende GitHub-Actions-Build-/Release-Konfiguration prüfen.
2. Versionierung, Tag-Schema und Stable-/Beta-Kanäle festlegen.
3. Freigabeschwelle: Tests, Signaturen und manuelle Abnahmen definieren.
4. Vorhandene Workflows für Linux-/Windows-/macOS-Builds aktualisieren.
5. x86-64- und ARM64-Plattformpakete korrekt zuordnen.
6. Plattformabhängige Paketierung/Installer in vorhandenen Skripten pflegen.
7. Windows-Signierung und macOS-Signierung/Notarisierung berücksichtigen.
8. Release-Hashes, Signaturen und SBOM/Lizenzinformationen prüfen.
9. GitHub Release automatisch nach erfolgreichem Tag/Workflow veröffentlichen.
10. Verifizierte Assets und maschinenlesbare Version-Metadaten bereitstellen.
11. NexVerse-Portal über offiziellen GitHub-Releases-Endpunkt anbinden.
12. Backend-Caching/Rate-Limits und Fehlerfälle der Release-API abfangen.
13. Portal-Downloadlinks nach Betriebssystem und Architektur auswählen.
14. Releasekanäle und Mindestversion aus Phase F sauber trennen.
15. Viewer-Update-Abfrage auf signierte Veröffentlichungen beschränken.
16. Download-/Installations-/Rollback-Tests und Offline-Fallback ausführen.
17. Verwaiste Build-Artefakte und doppelte UI-/Quellbestände inventarisieren.
18. Nur nach Abhängigkeitsprüfung alten ungenutzten Bestand entfernen.
19. Git-/CI-Prüfung auf Dateiduplikate und Altlasten in jeder /goal-Runde.
20. Vollständige OS-/CPU-Abnahme, Portaltest und Status-/Roadmap-Freigabe.

**Abnahme:** GitHub veröffentlicht nach bestandener CI echte
freigegebene Downloads. Das Portal zeigt automatisch den
korrekten verifizierten Release an. Es gibt keine automatisch
freigegebenen ungeprüften Nightly-Builds. Der Server bleibt
die Autorität über verpflichtende Mindestversionen. Keine neuen
Quell-/Ersatzdateien ohne ausdrückliche Zustimmung.

## Wartungsregel – für jede /goal-Phase obligatorisch

- **Vor jeder Arbeit:** vorhandene Dateien/Dateipfade, Status und
  Abhängigkeiten prüfen; kein neues Parallelprojekt erzeugen.
- **Währenddessen:** bestehende Dateien aktualisieren,
  fehlerhaften/überholten Code im gleichen Pfad ersetzen.
- **Nach jedem 20er-Block:** Duplikat-/Altlastenprüfung,
  Regressionstests, Aufräumen nur nicht mehr benutzter Teile.
- **Vor GitHub-Push:** Kein übrig gebliebener Test-/Debug-/
  Backup-Dateibestand und keine unreferenzierten UI-Komponenten;
  keine Löschung noch notwendiger Firestorm-Komponenten, Upstream-
  Copyrights oder Nutzerdateien.
- **Nicht automatisch im Hintergrund:** Regelmäßige Prüfung heißt
  bei jeder aktiven Entwicklungsphase und im CI-Workflow,
  nicht eine zugesagte Arbeit außerhalb dieser Unterhaltung.

## Prüfung

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    bash ogl/scripts/build-test.sh
    bash ogl/scripts/run-viewer-test.sh

Die echte Login-Auswahl ist noch visuell abzunehmen. Beim Klick auf
Welt betreten mit Beta darf derzeit kein Passwort übertragen werden.
