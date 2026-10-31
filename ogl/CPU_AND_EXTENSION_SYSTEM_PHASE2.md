# OGL Firestorm – CPU/ARM64 und Plugin-/Theme-Grundsystem, Phase 2

Datum: 31.10.2026 | Branch ogl-develop

## Architekturvorgabe

OGL Viewer muss fuer Linux, Windows und macOS entwickelt werden.
AMD und Intel benutzen beide typischerweise die x86-64-ISA.
ARM64 ist eine andere ISA (AArch64); darum brauchen wir
andere Maschineninstruktionen und passende Binaerpakete.
Keine globalen -march=native oder AVX2-Einstellungen in
dem portablem Basiscode. Die Runtime entscheidet nach
CPU- und Betriebssystemfaehigkeiten.

## Native CPU-Unterstuetzung

Datei: ogl/modules/include/ogl/ogl_cpu_features.h

- x86-64 und x86: CPUID prueft SSE2, AVX, AVX2.
- AVX2 darf nur genutzt werden, wenn OSXSAVE aktiv und
  XGETBV(XCR0) sowohl XMM als auch YMM sichert.
- ARM64/AArch64: Baseline NEON/Advanced SIMD. Bei abweichenden
  Compiler-Headern wird sicher der skalare Pufferpfad gebaut.
- Unbekannte CPU: ausschliesslich skalare Routine.
- Keine CPU-Herstellerkennung/-bevorzugung erforderlich.
- Bei macOS Universal ARM64+x86-64 wird eine architekturspezifische
  Kompilierung fuer beide Slices vermieden. Universal-Basis
  bleibt portabel bis getrennte Slices verifiziert sind.

Dateien: ogl/modules/cpu/buffer_dispatch.cpp,
buffer_avx2.cpp und buffer_neon.cpp sowie Tests.

Als erste tatsaechliche SIMD-Funktion ist
count_changed_bytes(a,b,n) mit skalarem Referenzpfad,
SSE2-, AVX2- und NEON-Pfad implementiert.
Die Funktion kann spaeter beispielsweise bei Cache-/
Textur-Dirty-Checks eingesetzt werden. AVX2 erhaelt
eine EIGENE C++-Uebersetzungseinheit mit -mavx2
bzw. /arch:AVX2; kein illegal-instruction-Crash beim
Start auf aelteren CPUs. Native Tests vergleichen
Vektorergebnisse mit der skalaren Referenz ueber
viele Laengen, Offsets und Aenderungsarten.

Diese Routine ist NOCH NICHT im produktiven Firestorm-Texturcache
verdrahtet. Daraus darf kein Viewer-FPS-Gewinn behauptet werden.

## Modul-/Plugin- und Theme-Verwaltung

Die versionierte OGL-C-ABI-Probe ist portabel fuer
Linux, Windows und macOS vorhanden. Noch keine
unbeschraenkten Drittanbieter-Nativmodule im Viewer laden:
DLL/.so/.dylib aus unbekannten Quellen laufen im
gleichen Prozess mit hohen Rechten.

Die Phase ergaenzt einen funktionalen, plattformneutralen
Theme-Dateimanager:
ogl/plugins/tools/manage_themes.py

Operationen: install, list, activate, reset, remove.
Es gilt ausschliesslich der vorher bereits gepruefte
.ogltheme-Manifest- und ZIP-Dateivertrag. Keine
Shell-Kommandos oder ausfuehrbaren Dateien.
Schutz: Pfadtraversal, Symlink-Verzeichnisse, Dateigroessen,
PNG-Signatur und Bilddimensionen, fehlerhafte ZIPs,
unbekannte Farb-Tokens, versehentliches Ueberschreiben,
atomische Aktivierungsdatei und getrennte Benutzerordner.

Datendatei:
- Linux: $XDG_DATA_HOME/ogl-viewer oder ~/.local/share/ogl-viewer
- Windows: %APPDATA%/OGL Viewer
- macOS: ~/Library/Application Support/OGL Viewer

Die Theme-Auswahl wird als Metadatum verwaltet; Firestorm
rendert die neuen Farb-Tokens NOCH NICHT aus der
Aktivierungsdatei. Ein Viewer-integrierter Installationsdialog,
Preview, Rollback und ein sandboxed Code-Plugin-Manager
sind noch zu bauen. Das Originaldesign bleibt unveraendert.

### Beispiel (nur isolierter Entwickler-Test, keine UI-Aktivierung)

    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    cd ogl/plugins/examples/classic-firestorm
    python3 -m zipfile -c /tmp/ogl-example.ogltheme theme.json
    cd ../../../..
    python3 ogl/plugins/tools/validate_theme.py /tmp/ogl-example.ogltheme
    python3 ogl/plugins/tools/manage_themes.py --data-dir /tmp/ogl-theme-demo install /tmp/ogl-example.ogltheme
    python3 ogl/plugins/tools/manage_themes.py --data-dir /tmp/ogl-theme-demo list
    python3 ogl/plugins/tools/manage_themes.py --data-dir /tmp/ogl-theme-demo activate de.nexverse.theme.classic-firestorm
    python3 ogl/plugins/tools/manage_themes.py --data-dir /tmp/ogl-theme-demo reset
    python3 ogl/plugins/tools/manage_themes.py --data-dir /tmp/ogl-theme-demo remove de.nexverse.theme.classic-firestorm

## Cross-OS CI-Matrix

OGL-Kernmodule, ABI, Python-Theme-Sicherheitstests,
CPU-Funktionserkennung und Puffervergleich auf:
- Ubuntu 24.04 x64 (AMD oder Intel)
- Ubuntu 24.04 ARM64
- Windows Server 2022 x64
- Windows 11 ARM64
- macOS 15 Intel x64
- macOS 15 Apple Silicon ARM64

Alle Runner-Labels sind laut GitHub Actions Dokumentation
verfuegbar. Ein erfolgreiches Linux-CI-Ergebnis ersetzt
NICHT die ARM-/Windows-/macOS-Tests. Der separate
Firestorm-Vollviewer verwendet derzeit noch x86-/OS-
spezifische Abhaengigkeiten: ein vollstaendiger
Windows-ARM64/Linux-ARM64-Release ist NICHT bewiesen.

GitHub-Quelle zu Runnern:
https://docs.github.com/en/actions/reference/runners/github-hosted-runners

## Abnahmebefehle (lokaler Linux Build)

    cmake -S ogl/modules -B .build-ogl-modules
    cmake --build .build-ogl-modules --parallel
    ctest --test-dir .build-ogl-modules --output-on-failure
    .build-ogl-modules/ogl_cpu_probe
    .build-ogl-modules/ogl_cpu_buffer_test
    python3 -m unittest discover -s ogl/tests -p 'test_*.py' -q
    bash ogl/scripts/build-test.sh

## Naechste technische Schritte

1. Reale CPU-Pufferfunktion in geeigneten Cache-/Bild-Pfad
   integrieren, ohne die originalen Firestorm-Funktionen
   global zu ersetzen.
2. Benchmarks fuer GPU-/CPU-/Speicherverbrauch auf x86-64
   (Intel/AMD) und ARM64, mit klar definiertem Fallback.
3. Live-Theme-Auswahl in Firestorm inklusive Preview,
   Aktivieren/Zuruecksetzen und per-User Design-Tokens.
4. Sandboxed Funktionsplugin-Runtime und Berechtigungsmodell.
5. Signierte, paketierte Releases auf allen drei OSen.
