# OGL Firestorm – P26: RTX-Erstscan bei niedriger OpenSim-Bildrate

Stand 31.10.2026, Linux Mint, Repository ogl-viewer, Branch ogl-develop.

## Live-Abnahme

Der zunächst gemeldete Test-Report zeigte 3D-Welt und funktionierende
RTX-Hardware, aber noch keine Szene und kein Depth-Readback. Im vollständig
ausgelesenen Linux-Mint-Log wurde die Ursache deutlich:

- 20:38:33 UTC: 3D-Welt gestartet und OGL Kamera/Sonnen-Bridge verbunden.
- Anfangs protokollierte Bildrate: 0,10 FPS.
- 20:40:48 UTC: nach 135 Sekunden FIRST GPU-Szene mit 8192 Dreiecken,
  Tiefen-Readback mit 47 GL-Oberflächen.
- Späterer, abgeschlossener Vulkan-GPU-Auftrag: erfolgreich.
- 42/47 primäre Vulkan-Treffer, 27/47 passende Tiefen, 0/27
  validierte Schattenstrahlen in der eingefrorenen 8x8-Diagnose.
- Kein neuer GPU-Worker-Fehler. Der Nutzer-Bericht wurde vor dieser
  Szene ausgeführt und war daher zeitlich zu früh.
- Mit wachsender Objektliste wurden 4 CPU-Snapshots veröffentlicht.
- Die dynamische Welt-Schattenbrücke bleibt leer, weil alte und neue
  Szenenrevisionen/Kamerabilder nicht strikt übereinstimmen.
  Das ist kein neuer Vulkan-Fehler, sondern bewusst fail-closed.

## Korrektur

Die ursprüngliche Bedingung für den OpenSim-Szenenscan war
ausschließlich 180 gerenderte Bilder. Bei initialen 0,1 FPS konnte
dies den ersten RTX-Scan um Minuten verzögern.

Im ausdrücklich aktivierten RTX-Entwicklermodus ist nun zusätzlich
ein zeitbasierter Erstauslöser vorhanden:
- Nur nach Erkennung einer wirklichen Region mit geladenen Objekten.
- Ab 5 Sekunden nach Ankunft, wenn seit dem letzten Versuch mindestens
  10 Sekunden vergangen sind.
- 180 Bilder bleiben als regulärer Auslöser bestehen.
- Frühere erfolglose Scans können bei noch leerer Szene erneut versucht
  werden; dafür gilt ebenfalls die 10-Sekunden-Abklingzeit.
- Wächst die geladene Szene um mindestens 25 Prozent bzw. 128 Objekte,
  wird eine begrenzte zusätzliche Neuauswahl zugelassen.
- Der normale OpenGL-Modus behält die alte 180-Bilder-Grenze,
  ohne zusätzliche Szenenscan-Arbeit.
- Regionswechsel setzt den Startzeitpunkt, Zähler und die
  Startup-Snapshot-Markierung zurück.

Neue numerische Logzeile:
OGL RTX startup scene scan: cause=first_snapshot|object_growth
frames=N loaded_objects=N region_age_ms=N

Der lokale Report unterscheidet nun, ob bereits eine Kamera-Bridge
besteht, noch kein Meshscan stattgefunden hat oder ein früher
zeitgesteuerter Scan ausgelöst wurde. Reale GPU-Proben, deferred
depth und world-shadow-Komposition bleiben getrennte Status.

## Tests und Grenzen

Das Scheduling wurde als testbarer C++-Header mit nativen Regressionen
implementiert. Vollständiges Bauen und bestehende Vulkan-Hardwaretests
sind notwendig. Eine weitere manuelle OpenSim-Anmeldung ist weiterhin
erforderlich, weil der Simulator keine lokalen Testdaten liefert.

Die GPU-Abdeckung bleibt bewusst auf höchstens 8192 Dreiecke und eine
8x8-Entwicklerprobe begrenzt. Das behebt ausschließlich die verzögerte
Szenenerfassung, nicht fehlende In-World RTX-Schatten oder die HTTP-502-
Texturprobleme.

## Abnahme

Bisherige Instanz vollständig schließen, dann:

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    bash ogl/scripts/run-rtx-preview-test.sh

Nach Weltstart:

    python3 ogl/scripts/ogl-test-report.py

Erwartet wird eine Logzeile RTX startup scan vor dem alten
180-Frames-Limit bei langsamem Laden oder bei signifikant wachsender
Objektliste, gefolgt von RTX world geometry sample und (bei
ausreichenden geladenen Daten) einem erfolgreich abgeschlossenen
RTX GPU diagnostic. Sichtbares Raytracing bleibt nicht implementiert.
