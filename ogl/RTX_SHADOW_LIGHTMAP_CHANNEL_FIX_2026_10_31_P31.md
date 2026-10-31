# OGL Firestorm – P31: Richtige Sonnenlichtkanäle und echter Farbvergleich

31.10.2026 | Linux Mint | Branch ogl-develop

## Ausgangsbefund

Der Nutzer sieht trotz erfolgreicher Vulkan-Ray-Queries keinen RTX-
Unterschied. Im letzten Live-Bericht wurden 614 GPU-Schattenpixel,
5747 tiefenvalidierte Samples und 259561 OpenGL-Fragmente gemeldet.
Diese GL-Fragmente gehören zum Sonnenlicht-Renderpass, nicht
automatisch zum finalen Bildschirmbild.

## Festgestellte konkrete Ursache

Firestorms GLSL-Shader der Lightmap ist eindeutig kanalbasiert:
- class2/deferred/sunLightF.glsl setzt R auf die direkte
  Sonnenschatten-Sichtbarkeit, G auf 1.0/SSAO, B/A auf Spot-Schatten.
- class3/deferred/softenLightF.glsl interpretiert R als Sonnenlicht
  und G als AO – nicht als RGB-Farbe.

Der vorherige Vulkan-Compositor schrieb mit Alpha-Blending auf
alle Kanäle und versuchte sogar, die Lightmap rot zu färben.
Rot an dieser Stelle ist kein roter Bildschirm-Pixel, sondern
ein modifizierter Sonnenlicht-Skalar. Die Veränderung konnte
so normal beleuchtete/SSAO-Bestandteile unbeabsichtigt beeinflussen.

## Umgesetzte Korrektur

1. Das normale Vulkan-zu-OpenGL-Compositing dunkelt nur den R-Kanal
   der Deferred-Sunlight-Lightmap ab.
2. G, B und A bleiben unverändert (SSAO, Spotlights, andere Daten).
3. Der ursprüngliche GL_COLOR_WRITEMASK wird explizit erfasst und
   nach dem Renderdurchlauf wiederhergestellt, wie zuvor bereits
   Blend-, Depth-, Scissor-, Texture- und Program-States.
4. Der Debug-Modus wird als zweiter, ausschließlich opt-in
   Pass auf dem abschließenden 3D-Farbbild des Viewers gerendert.
   Der rote Pixel stammt damit wirklich aus einer RGB-Overlayfarbe,
   nicht aus einem irrtümlich als Farbe behandelten Schattenkanal.
5. Die Debug-Ebene wird nach atmosphärischem Licht, lokalen Lichtern
   und alpha-Geometrie, vor dem Flush der 3D-Farbdaten gezeichnet.
6. Der gleiche strenge Kamera-/Geometrie- und Live-Depth-Check gilt
   für beide Passes. Nur bestätigte Vulkan-Schatten erhalten eine
   farbige Markierung.
7. Die normale Schatten-Fragmentmessung (GL_SAMPLES_PASSED)
   bleibt auf den R-Kanal-Lightmap-Pass beschränkt; sie wird nicht
   durch eine zweite Debug-Messung vermischt.
8. Der Testbericht hat nun eine separate Diagnose-Zeile
   RTX diagnostic RED overlay, falls die rote Markierung
   tatsächlich als 3D-Farb-Drawcall eingereicht wurde.
9. Portabler Python-Regressionstest überprüft die
   kanalbasierte Firestorm-Shader-Semantik, das isolierte
   Farbmasken-Schreiben und die Position des Debug-Passes.
10. Alle nativen Tests, Python-Tests, GLSL-Checks, Viewer-Build
    und GitHub-Synchronisierung durchgeführt.

## Sicherheitsgrenzen und nächste Abnahme

Dies ist noch immer experimentelles 128x72-Hybrid-Raytracing aus
höchstens 8192 GPU-Dreiecken. Die Auflösung ist keine hochauflösende
Schattenkarte, und Pathtracing / Reflexionen / indirektes Licht
sind nicht implementiert. Der Debug-Overlay-Pass beweist eine
beabsichtigte 3D-Farb-Ausgabe; erst ein echtes Nutzerbild beweist,
dass die nachfolgenden Firestorm-Postprocessing-Shader sie erhalten.

Alten Viewer schließen, neuer Diagnose-Test:

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    bash ogl/scripts/run-rtx-shadow-visual-test.sh

Danach:

    python3 ogl/scripts/ogl-test-report.py

Erwartet:
- RTX OpenGL fragment coverage: ...
- RTX diagnostic RED overlay: submitted to actual 3D color target ...

Wenn die rote Markierung fehlt, trotz erfolgreichem Log,
Screenshot erstellen; dann muss die nachfolgende
Render-Target-Kopie/Postprocessing-Ausgabe geprüft werden.

Normaler Test ohne roten Vergleich:

    bash ogl/scripts/run-rtx-dense-preview-test.sh
