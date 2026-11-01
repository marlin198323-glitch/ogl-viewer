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
8. **Design und Plugins:** Das originale Firestorm-Design bleibt
   Standard; optionale Designs, Templates und Erweiterungen
   müssen sicher und rücksetzbar sein.
9. **Projektgedächtnis im Repository:** Nach jedem bearbeiteten
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
- Das originale Firestorm-Design bleibt der Standard.
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
Live-Vorschau und Original-Rücksetzung. Chat-, IM- und Gruppenfenster
einheitlich mit rundem Avatarbild, Namen, Uhrzeit und Text darstellen.
Multi-DPI und kleine/große Auflösungen testbar gestalten.

## Phase D – abgesicherter Plugin-Manager

Manifest, Capabilities, Signaturen, Sandbox, API-Version, Benutzer-
freigabe, Update und Widerruf. Keine ungeprüften DLL/SO/DYLIB laden.

## Phase E – CPU/GPU, Cache, Grafik und Releases

Portable CPU-Wege scalar/SSE2/AVX2/ARM64-NEON, Grafikfähigkeiten
dynamisch erkennen und alte Hardware über OpenGL absichern.
Textur-, Mesh- und Avatar-Ladezeiten messen und verbessern.
Vulkan-RT-Tests nur optional. Linux-, Windows- und macOS-Builds
einschließlich x64 und ARM64 separat bauen, signieren und testen.

## Phase F – verpflichtender Viewer-Updater

Die verbindliche Mindestversion muss vor dem Login geprüft werden.
Der Viewer bietet Pflichtupdates an, der NexVerse-Server lehnt
nicht mehr zugelassene Versionen ab. Das System muss Linux,
Windows und macOS samt x64-/ARM64-Paketen unterstützen.
20 Entwicklungsblöcke und Abnahmekriterien stehen in
GOAL_MANDATORY_VIEWER_UPDATER.md.

**Status: geplant, nicht implementiert.**

## Prüfung

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    bash ogl/scripts/build-test.sh
    bash ogl/scripts/run-viewer-test.sh

Die echte Login-Auswahl ist noch visuell abzunehmen. Beim Klick auf
Welt betreten mit Beta darf derzeit kein Passwort übertragen werden.
