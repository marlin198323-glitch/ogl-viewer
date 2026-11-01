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

**Veröffentlichungsquelle:** das kanonische GitHub-Repository
https://github.com/marlin198323-glitch/ogl-viewer veröffentlicht
nach bestandener CI automatisch signierte Versions-Releases über
GitHub Releases. Der Viewer lädt passende Pakete nur aus einer
vertrauenswürdig geprüften Releasequelle. Das Stadt-NexVerse-Portal
bezieht die Release-Informationen von GitHub automatisch über
die offizielle Releases-API bzw. einen serverseitig geprüften
Manifest-Cache, mit Rate-Limit- und Ausfallbehandlung.

Die Update-Dateien müssen signiert und geprüft sein. Benutzerprofile,
Cache, Anmeldedaten, Themes und Plugins bleiben bei Updates erhalten.
Linux, Windows, macOS und passende AMD-/Intel-x64-/ARM64-Pakete
werden mit gleichem Funktionsumfang unterstützt.

**Trennung von Anzeige und Zwang:** GitHub ist die
Artefakt-/Versionsquelle für Downloads; der NexVerse-Server
bestimmt selbst, ab welcher Mindestversion er den Zugang
erlaubt. Ein abgebrochener Download darf niemals die
serverseitige Versionskontrolle aushebeln. Bei nicht
erreichbarem GitHub sichere, bereits verifizierte Metadaten
oder eine vertrauenswürdige Fehleranzeige nutzen; keine
willkürlichen Drittanbieter-URLs installieren.

## Bereits vorhandene Grundlage

Firestorm enthält Velopack-Quellcode für Windows und macOS
sowie Code zur Behandlung verpflichtender Updates.
Eine ältere Updateprüfung in llappviewer.cpp ist deaktiviert.
Eine vollständige OGL-Pflichtupdatefunktion einschließlich
Serverprüfung und Linux-Paketen existiert noch nicht.

## /goal Updater – 20 geplante Entwicklungsblöcke

1. GitHub-Versionstags und stabile/Beta-Releasekanäle festlegen.
2. Gemeinsame Mindestversions-API zwischen Client und Server.
3. Linux/Windows/macOS x64/ARM64-Paketmatrix für GitHub Releases definieren.
4. Vorhandenen Velopack-Code prüfen und für OGL anpassen.
5. Aus GitHub Releases geprüftes Manifest mit Artefaktmetadaten erzeugen.
6. Release-/Manifest-Signaturen, erlaubte Hosts und Herkunft prüfen.
7. HTTPS-, GitHub-API-Rate-Limit-, Cache- und Fehlerbehandlung implementieren.
8. Versionen korrekt vergleichen und Pflichtupdate erkennen.
9. Aktualisierung vor manuellen und automatischen Logins prüfen.
10. Pflichtupdate-Dialog mit Versions- und Fortschrittsanzeige.
11. GitHub-Release-Downloads und Wiederholungen mit Fortschritt.
12. Hash-, Signatur-, OS-/CPU-Architektur- und Paketprüfung.
13. Installation, Neustart und abgesicherte Wiederherstellung.
14. Windows x64/ARM64-Pakete per CI bauen, signieren und veröffentlichen.
15. macOS Intel/Apple-Silicon per CI paketieren, signieren und notarisieren.
16. Linux x64/ARM64 per CI paketieren und Release-Assets erstellen.
17. Mindestversion im öffentlichen OpenSim-Login durchsetzen.
18. Mindestversion im separaten Beta-Login durchsetzen.
19. Portal bezieht GitHub-Release-Daten; alte Clientversion gesperrt, aktuelle zugelassen.
20. Plattform-/Portal-/Updater-Livetests und automatische GitHub-Freigabe.

Ohne abgeschlossene Client-/Server-Integration ist das
Pflichtupdate noch keine produktiv freigegebene Funktion.
