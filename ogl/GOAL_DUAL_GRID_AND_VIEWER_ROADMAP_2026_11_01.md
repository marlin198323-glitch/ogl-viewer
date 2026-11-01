# /goal – OGL Firestorm: plattformübergreifender Entwicklungsplan

Stand: 01.11.2026 | Hauptbranch: ogl-develop

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

## Prüfung

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    bash ogl/scripts/build-test.sh
    bash ogl/scripts/run-viewer-test.sh

Die echte Login-Auswahl ist noch visuell abzunehmen. Beim Klick auf
Welt betreten mit Beta darf derzeit kein Passwort übertragen werden.
