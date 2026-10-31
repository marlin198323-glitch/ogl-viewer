# OGL Firestorm – echter Raytracing-Renderer, Phase 1: GPU->Welt-Schattenquelle

Stand 31.10.2026, Linux-Mint-Entwicklung, Branch ogl-develop.
Technologie: OpenGL als bestehender Firestorm-Renderer, optionale
Vulkan Ray Queries für echte Sonnenstrahlen als Hybrid-Renderer.

## In dieser Phase implementiert

- Welt-Schatten-API unabhängig von der bereits existierenden 8x8-HUD-Debug-Maske.
- Versionierter Datentyp ogl_rtx_world_shadow_sample.
- Bei Vulkan-Ray-Queries wird je Kamera-Strahl der tatsächlich mit
  OpenGL-Tiefe übereinstimmende primäre Geometrietest übernommen.
- Schattenbits gelten nur, wenn der Primärstrahl dieselbe Weltoberfläche
  wie OpenGL trifft.
- Der asynchrone GPU-Worker trägt die unveränderliche Kamera/FOV/
  Sonnenposition, originale Tiefenwerte und die Tiefenübereinstimmungsmaske.
- Der Mainthread veröffentlicht bei erfolgreichem GPU-Abschluss eine
  separate Welt-Schattenquelle, ausschließlich wenn aktuelle Geometrie-
  revision und Kamera mit dem GPU-Auftrag übereinstimmen.
- Regionwechsel, Geometrieänderungen, Kamerabewegungen und neue
  Tiefen-Captures invalidieren die Welt-Schattenquelle automatisch.
- Der Firestorm-Bildrenderpfad prüft diesen Datensatz über
  ogl_rtx_world_shadow_source_ready und meldet nur einen gültigen Eingang.
- Portabler C++-Test prüft Versionen, Kamera, Projektion, Bildmaße,
  Lichtvektor, Geometrie, Sky-Samples und abweichende Tiefenwerte.
- Ohne passende Originaltiefe und Bildpixel ist keine Welt-Schatten-
  Übertragung zulässig; 8x8-Samples werden nicht unkontrolliert gestreckt.
- Die vorhandene HUD-Debuganzeige bleibt separat.
- Public OpenGL, Cache, Authentifizierung und Simulator unverändert.

## Entscheidende Grenze

**Noch keine RTX-Schatten in der sichtbaren 3D-Welt.**
Der aktuelle GPU-Prototyp berechnet nur 8x8 Schattenproben und wird
asynchron/gedrosselt ausgeführt. Eine Vergrößerung dieser Proben auf
den gesamten Bildschirm wäre ungenau und bei Bewegung zeitversetzt.
Deshalb bleibt rendering_active=0 und OpenGL der aktive Renderer.
Das neue Feature ist die geprüfte Schnittstelle, über die ein echter
Schatten-Compositor später die richtigen GPU-Ergebnisse erhält.

## Technischer Fahrplan

1. Dynamische GPU-Schattenbildgröße (zunächst 64x36 oder 128x72,
   unabhängig von der bisherigen 8x8-Debug-ABI).
2. Aktuelle GL-Depth-Revalidierung pro schattiertem Pixel und exakte
   Kamera-/Frame-/Geometrie-Synchronisierung.
3. Persistente BLAS/TLAS für mehr echte Mesh- und Terrain-Geometrie;
   rigged Avatare, Alpha-Masken und Bewegung getrennt behandeln.
4. Sicheres Vulkan/OpenGL-Interop oder asynchrones PBO-/Textur-Staging
   mit Ressourcen-Fences und Ausfallsicherung.
5. Opt-in GPU-Sonnen-Schatten-Komposition im Weltbild mit GBuffer-
   Materialvalidierung, Shadow Bias, Denoising und Stabilisierung.
6. Anschließend echte Reflektionen und Global Illumination sowie
   Licht-/Material-PBR und hardwareunabhängige Qualitätsstufen.

## Manuelle Linux-Mint-Abnahme

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    bash ogl/scripts/run-rtx-preview-test.sh
    python3 ogl/scripts/ogl-test-report.py

Neue Berichtzeile: RTX world-shadow bridge. Der Bericht enthält weiterhin
Ray-traced world rendering: not implemented, bis wirklich vollständige
sichtbare Schatten in die Welt composited werden. Keine vorgetäuschte
Renderfreigabe.
