# OGL Firestorm – aktueller Projektstand

Stand: 01.11.2026
Repository: https://github.com/marlin198323-glitch/ogl-viewer
Branch: ogl-develop
Verbindlicher Auftrag: GOAL_DUAL_GRID_AND_VIEWER_ROADMAP_2026_11_01.md
Übernahme in neuen Chats: PROJECT_HANDOFF.md und AGENTS.md (Repo-Wurzel)

## Stand der /goal-Phasen

| Phase | Stand | Nächster überprüfbarer Schritt |
| --- | --- | --- |
| Phase A – zwei feste Grids | 19 von 20 Blöcken im Code/Build geprüft; visuelle Abnahme ausstehend | Beide Login-Grid-Einträge und öffentlichen OpenSim-Login im laufenden Viewer testen |
| Phase B – Beta-Protokoll | Geplant | TLS-Zertifikat und tatsächliches Login-Protokoll verifizieren; ohne Bestätigung keine Passwortübertragung |
| Phase C – Designs, Templates, Chat | Designsystem Phase 1 mit Farbwahl implementiert; native Installation und weitere Templates offen | Nutzerabnahme der OGL-Designs und später sichere GUI-Paketinstallation |
| Phase D – Plugin-Manager | Grundlage vorhanden, vollständiger sicherer Manager geplant | Versionierte Plugin-API, Rechte, Isolation und Signaturen konzipieren |
| Phase E – CPU/GPU, Cache, Grafik | Verschiedene Tests/experimentelle Module implementiert, Gesamtziel offen | Grafik-/Cache-Livebenchmark und vollständige Plattform-Builds prüfen |
| Phase F – verpflichtender Updater | Geplant, 0 von 20 Blöcken implementiert | Gemeinsame Client-/Server-Mindestversions-API und Releasekanäle festlegen |

## Fest bestätigte technische Grundlage

- Der Quellcode für die zwei festen NexVerse-Grids liegt auf ogl-develop.
  Stadt NexVerse Beta ist auswählbar; der eigentliche Beta-Login bleibt
  bis zur abgesicherten Zertifikats- und Protokollintegration gesperrt.
- Zum Grid-Umbau wurde auf Linux Mint ein vollständiger Viewer-Build
  einschließlich 119 Python-Tests und 41 nativen Tests erfolgreich
  gemeldet. Die sechs Varianten der Modul-/CPU-CI waren erfolgreich.
  Diese Angaben beziehen sich auf den damaligen Code-Teststand
  und sind keine pauschale Freigabe aller OS-Installer.
- Im OGL-Designsystem sind Theme-Auswahl, Farb-Vorschau,
  Farb-Aktivierung und Wiederherstellung eingebaut; das
  originale Firestorm-Design bleibt der Standard.
- Die Pflichtupdate-Policy ist ausschließlich als Anforderung
  und 20-Block-Plan dokumentiert. Es ist noch **kein**
  produktiv durchsetzender Viewer-/Server-Updater vorhanden.
- Ein früherer OpenGL/Vulkan-RTX-Entwicklungsstand enthält
  Diagnosen; sichtbare Welt-RX/RTX-Schatten sind nicht
  automatisch als implementiert oder freigegeben anzusehen.

## Betriebssysteme und Architektur

Verbindliche Ziele: Linux, Windows, macOS sowie x86-64
(AMD/Intel) und ARM64. Gemeinsame Funktionalität, automatische
CPU-/GPU-Erkennung und kompatibler OpenGL-Standardpfad.
Ein grüner Modul-Test ist **nicht** dasselbe wie ein gebauter,
signierter und manuell getesteter vollständiger Viewer.

## Verbindliche Pflicht-Updater-Anforderung

Sobald eine neue Mindestversion für Stadt NexVerse oder
NexVerse Beta vorgeschrieben ist, muss eine ältere Version
vor dem Server-Login aktualisiert werden. Ablehnen/Abbrechen
erlaubt keinen Zugang; der Server prüft die Mindestversion
selbst. Der genaue 20-Block-Plan steht in
GOAL_MANDATORY_VIEWER_UPDATER.md.

## Vorgehen für jede weitere 20-Block-Runde

1. AGENTS.md, PROJECT_HANDOFF.md, diese Statusdatei und
   die relevante /goal-Roadmap lesen.
2. Über Linux Mint Commander git status, aktuellen Branch
   und den letzten Commit prüfen; lokalen Quellstand schützen.
3. Einen genauen 20-Block-Umfang mit Abnahmekriterien wählen.
4. Implementieren und nach Möglichkeit den echten Viewer
   bauen, Python-/native Tests und nötige Plattform-CI prüfen.
5. Alle tatsächlichen Ergebnisse dokumentieren; manuelle
   Sicht-/Login-Tests ausdrücklich als offen kennzeichnen.
6. Fertige, geprüfte Änderungen sofort auf ogl-develop pushen.
7. **Diese Datei aktualisieren:** Status je Phase, erledigte
   Blöcke, Teststände, neuer Commit, offene Fehler, konkrete
   nächste Schritte. Kein willkürliches Markieren als fertig.

Wird ein neues Chatfenster eröffnet, muss mindestens die
Repository-URL plus Branch angegeben und der Assistent zum
Einlesen der obigen Projektdateien aufgefordert werden.
Projektdateien allein übertragen den Kontext nicht
automatisch in jede neue Chat-Unterhaltung.
