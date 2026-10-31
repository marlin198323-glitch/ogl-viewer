# NexVerse Cinematic – Bildqualität wie im Referenzbild

Das hochgeladene Referenzbild zeigt eine aufwendig gestaltete Szene mit
goldenem Sonnenuntergang, indirekter Beleuchtung, poliertem Boden, Lampen,
hochwertigen Architektur-Assets und großen Wasserreflexionen.
**Raytracing ist dafür hilfreich, aber allein nicht ausreichend**.
Kein Viewer kann aus niedrig aufgelösten Texturen, fehlenden PBR-Materialien
oder fehlender Weltgeometrie automatisch dieses Ergebnis erzeugen.

## Bereits möglich: OpenGL/PBR-Grafikprofil

OGL liefert einen optionalen Generator für ein natives Firestorm-Grafikpreset:
`ogl/scripts/install-cinematic-preset.py`.

Auf Linux Mint ist das Preset unter
`~/.local/share/ogl-firestorm-test/user_settings/presets/graphic/NexVerse%20Cinematic.xml`
installiert. Die bisher aktive Grafikkonfiguration wurde nicht verändert.

Das neue Profil schaltet 19 reguläre Firestorm-Grafikparameter in Richtung
fotorealistischer Qualität: Schattenauflösung, SSAO, Screen-Space-Reflexionen,
Reflexionsproben inklusive dynamischer Objekte, Wasserreflexionen, LOD,
anisotrope Filterung, SMAA, Draw Distance und Bloom.

Im Viewer über die **Grafikvoreinstellungen** das Profil
**NexVerse Cinematic** laden. Zurück wechseln über die vorhandene
Grafikvoreinstellung, z. B. **Default**. Bei ultrabreiter Auflösung
können FPS und VRAM-Verbrauch erheblich steigen.

Test auf anderen Linux-Mint-Rechnern mit diesem OGL-Profil:

```bash
cd "$HOME/Schreibtisch/OGL-Viewer/ogl-firestorm"
python3 ogl/scripts/install-cinematic-preset.py
python3 ogl/scripts/install-cinematic-preset.py --install
```

Der erste Aufruf ist nur ein Lesetest. Der zweite erstellt ausschließlich
eine neue Grafikpreset-Datei. Bereits vorhandene Presets werden **nicht**
überschrieben; keine Nutzer- oder Accountdaten werden in Git gespeichert.

## Für echte RTX-Grafik zusätzlich erforderlich

1. RTX-Modul: NVIDIA 4070 SUPER, Vulkan Ray Query und logisches Gerät sind
   bereits verifiziert.
2. OpenSim-Bestandsgeometrie in einer echten Welt-Session überprüfen.
   Der jüngste Diagnosebericht stammt vom **Loginfenster**; darin wurde
   `STATE_STARTED` nicht erreicht. Deshalb meldet die Diagnose korrekt
   noch keinen Dreiecks-Snapshot.
3. Gesamte sichtbare Szene statt eines 1.024-Dreiecks-Testlimits sammeln,
   lokale und globale Koordinaten korrekt transformieren und Änderungen
   nachführen (Terrain, Avatare, rigged Meshes, Wasser, Transparenz).
4. Vulkan BLAS/TLAS auf der GPU aufbauen, mit Budget und asynchronen
   Updates.
5. Raytracing-Schatten in einem Compute-Shader berechnen, zeitlich glätten,
   entrauschen und ins OpenGL-Bild einblenden.
6. Materialien und Reflexionen: PBR roughness/metalness, Spiegelungen auf
   Stein und Glas; Wasserreflexionen/Refraction und perspektivisch GI.
7. GPU-/FPS-Budget, Upscaling und Stufen Aus / Qualität / Ultra.
8. Umgebung vor Ort: hochwertig erstellte Texturen, Materialzuordnung,
   warme EEP-Himmels- und Wasserbeleuchtung sowie platzierte Lichtquellen.

Derzeit ist der echte RTX-Renderpfad nicht fertig.
`rendering_active=0` darf erst dann auf `1` wechseln, wenn nachweislich
RTX-berechnete Bildpixel in der Viewer-Szene erscheinen.

**OpenSim-Server:** Für den eigentlichen Client-Renderer ist keine Änderung
erforderlich. Die **Inhalte** der Welt können aber für die Bildqualität
Anpassungen benötigen (Licht, Material, Wasser, Texturen).

Primärquellen:
- https://docs.vulkan.org/tutorial/latest/courses/18_Ray_tracing/02_Acceleration_structures.html
- https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Advanced_Topics/Ray_Query_Rendering.html
- https://opensimulator.dev/wiki/Compatible_Viewers
