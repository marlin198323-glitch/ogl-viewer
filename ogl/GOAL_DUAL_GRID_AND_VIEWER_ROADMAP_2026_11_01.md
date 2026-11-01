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

## Phase A – zwei feste NexVerse-Login-Profile (20 Blöcke)

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

## Phase B – separates Beta-Protokoll (je 20 Blöcke)

Zertifikat und dokumentierten Auth-Vertrag verifizieren; modularen
OpenSim-/Beta-Loginadapter implementieren; Zugangsdaten,
Sessions und Tokens sauber isolieren. Region, Avatar, Inventar,
Chat, Teleport und Fehlerverhalten erst nach Protokollabnahme
über adapterbasierte APIs anschließen.

## Phase C – Designs, Templates und Chat

Bestehenden OGL-Designs-Reiter ausbauen, native sichere ZIP-Installation,
Live-Vorschau und Rücksetzung auf das **OGL-Standarddesign** ermöglichen.
Das originale Firestorm-Design darf nicht mehr als reguläre
Nutzeroberfläche erscheinen; technische/urheberrechtlich nötige
Referenzen bleiben erhalten. Chat-, IM- und Gruppenfenster
einheitlich mit rundem Avatarbild, Namen, Uhrzeit und Text darstellen.
Multi-DPI und kleine/große Auflösungen testbar gestalten.
Detaillierte, auf 20 Blöcke aufteilbare UI-Abnahmen: Phase H.

## Phase D – abgesicherter Plugin-Manager

Manifest, Capabilities, Signaturen, Sandbox, API-Version, Benutzer-
freigabe, Update und Widerruf. Keine ungeprüften DLL/SO/DYLIB laden.

## Phase E – CPU/GPU, Cache, Grafik und Releases

Portable CPU-Wege scalar/SSE2/AVX2/ARM64-NEON, Grafikfähigkeiten
dynamisch erkennen und alte Hardware über OpenGL absichern.
Textur-, Mesh- und Avatar-Ladezeiten messen und verbessern.
Vulkan-RT-Tests bleiben optional; **Vulkan als eigene,
auswählbare Grafik-Engine** wird gesondert in Phase G entwickelt.
Linux-, Windows- und macOS-Builds einschließlich x64 und ARM64
separat bauen, signieren und testen.

## Phase F – verpflichtender Viewer-Updater

Die verbindliche Mindestversion muss vor dem Login geprüft werden.
Der Viewer bietet Pflichtupdates an, der NexVerse-Server lehnt
nicht mehr zugelassene Versionen ab. Das System muss Linux,
Windows und macOS samt x64-/ARM64-Paketen unterstützen.
20 Entwicklungsblöcke und Abnahmekriterien stehen in
GOAL_MANDATORY_VIEWER_UPDATER.md.

**Status: geplant, nicht implementiert.**

## Phase G – auswählbare Grafik-Engines OpenGL und Vulkan (20 Blöcke)

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

## Phase H – OGL/NexVerse-Branding, Navigation und Login (20 Blöcke)

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

## Phase I – GitHub-Releases, Portal-Anbindung, Quellbaumbereinigung (20 Blöcke)

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
