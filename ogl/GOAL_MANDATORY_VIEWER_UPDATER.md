# /goal Pflicht-Updater – OGL Firestorm Viewer

Status: geplant, noch nicht implementiert.
GitHub: https://github.com/marlin198323-glitch/ogl-viewer
Branch: ogl-develop

## Verbindliche Anforderung

Nach einer neuen verpflichtenden Release-Version muss der Viewer das
Update vor der Anmeldung installieren. Wird das Update abgelehnt,
ist der Zugang zur NexVerse-Welt nicht erlaubt.

Das muss sowohl im Viewer als auch am Server umgesetzt werden.
Der Server kontrolliert die aktuell zugelassene Mindestversion
für das öffentliche OpenSim-Grid und das Beta-Grid getrennt.

Die Update-Dateien müssen signiert und geprüft sein. Benutzerprofile,
Cache, Anmeldedaten, Themes und Plugins bleiben bei Updates erhalten.
Linux, Windows, macOS und passende AMD-/Intel-x64-/ARM64-Pakete
werden mit gleichem Funktionsumfang unterstützt.

## Bereits vorhandene Grundlage

Firestorm enthält Velopack-Quellcode für Windows und macOS
sowie Code zur Behandlung verpflichtender Updates.
Eine ältere Updateprüfung in llappviewer.cpp ist deaktiviert.
Eine vollständige OGL-Pflichtupdatefunktion einschließlich
Serverprüfung und Linux-Paketen existiert noch nicht.

## /goal Updater – 20 geplante Entwicklungsblöcke

1. Versionierung und stabile/Beta-Releasekanäle festlegen.
2. Gemeinsame Mindestversions-API zwischen Client und Server.
3. Betriebssystem-/CPU-Paketmatrix verbindlich definieren.
4. Vorhandenen Velopack-Code prüfen und für OGL anpassen.
5. Release-Manifest mit Versionen und Artefaktmetadaten.
6. Manifest-Signaturen und vertrauenswürdige Quellen prüfen.
7. HTTPS-, Timeout- und Fehlerbehandlung implementieren.
8. Versionen korrekt vergleichen und Pflichtupdate erkennen.
9. Aktualisierung vor manuellen und automatischen Logins prüfen.
10. Pflichtupdate-Dialog mit Versions- und Fortschrittsanzeige.
11. Downloads und Wiederholungen mit Fortschrittsanzeige.
12. Hash-, Signatur-, Architektur- und Paketprüfung.
13. Installation, Neustart und abgesicherte Wiederherstellung.
14. Windows-x64-/ARM64-Paketierung und Tests.
15. macOS-Intel-/Apple-Silicon-Paketierung und Notarisierung.
16. Linux-x64-/ARM64-Verteilung und Updates.
17. Mindestversion im öffentlichen OpenSim-Login durchsetzen.
18. Mindestversion im separaten Beta-Login durchsetzen.
19. Abnahme: alte Version abgelehnt, neue Version zugelassen.
20. Plattformtests, Live-Abnahme und GitHub-Freigabe.

Ohne abgeschlossene Client-/Server-Integration ist das
Pflichtupdate noch keine produktiv freigegebene Funktion.
