# OGL Firestorm P37 – echte CPU-Verbindung zum schnellen Texturcache

Datum: 01.11.2026 | Branch: ogl-develop

## Ausgangspunkt

Die portable OGL-AVX2/NEON-Puffervergleichsroutine bestand
bisher nur als separate Testbibliothek. Firestorms echter
Fast-Texturcache schreibt feste Datensaetze mit 16 Byte
Metadaten und bis zu 1.024 Pixelbytes. Identische
Texturvorschauen koennten redundante Schreibvorgaenge
verursachen. Ein zusaetzlicher Lesezugriff kostet aber
ebenfalls Datentraegerzeit. Deshalb ist diese Aenderung
nur auf ausdruecklichen Testwunsch aktiv.

## 20 Arbeitsschritte

1. Quellcode der echten Textur- und Mesh-Caches geprueft.
2. Firestorms Fast-Cache-Schreib- und Lesewege verglichen.
3. Standardverhalten ohne zusaetzliche Lesezugriffe beibehalten.
4. Die CMake-CPU-Targets in eine zentrale portable Funktion
   ausgelagert und doppelte Compilerregeln vermieden.
5. Die CPU-Vergleichsbibliothek in den Firestorm-Build eingebunden.
6. AVX2 ausschliesslich in einer separaten x86-Uebersetzungseinheit.
7. SSE2 als kompatible x86-64-Basis, ARM64-NEON optional.
8. CMake-Universal-Build-Schutz fuer macOS beibehalten.
9. CPUID/OSXSAVE/XGETBV-Feature-Erkennung pro Prozess
   gecacht, damit sie nicht pro Eintrag wiederholt wird.
10. Reine C++-Hilfsfunktion fuer Cache-Metadaten und
    gueltige Bildbytes erstellt, ohne Dateiformat zu aendern.
11. Vergleich nicht initialisierter Tail-Bytes ausgeschlossen.
12. Native Grenz- und Bildvergleichstests hinzugefuegt.
13. Gemeinsamen Fast-Cache-Schreibpuffer gegen Race Conditions
    geschuetzt: Erstellung und Datei-I/O unter einem Mutex.
14. Read-before-write ausschliesslich bei gesetzter Variable
    OGL_FASTCACHE_SKIP_IDENTICAL=1 aktiviert.
15. Ungueltige, fehlende oder zu kurze Cacheeintraege
    weiterhin im originalen Firestorm-Verfahren geschrieben.
16. Keine neuen Bildkopien, zusaetzlich nur temporare
    1-KB-Lesedaten bei explizit aktivierter Optimierung.
17. Logstatistik ohne Texture-UUIDs oder Kontodaten:
    checks, skipped_writes, cpu_backend.
18. Testbericht gibt diese aggregierten Zaehler aus und
    betont, dass es keine gemessene FPS-Steigerung ist.
19. CPU-Mikrobenchmark mit 180.000 1-KB-Datensaetzen,
    davon 120.000 identisch und 60.000 veraendert;
    libc memcmp war in dieser Umgebung noch schneller als AVX2.
    Deshalb nutzt die Gleichheitspruefung jetzt libc memcmp.
20. Python-, native C++, Vulkan- und Gesamtviewer-Builds,
    Cross-Plattform-Fallbacks und GitHub-Freigabe pruefen.

## CPU-Messwert nur dieser Entwicklungsmaschine

Benchmark nach gecachter CPU-Faehigkeitserkennung:
180.000 Vergleiche, je 1.024 Bildbytes.
AVX2-Vergleich (ein Lauf): circa 19.892 Mikrosekunden.
Skalare Referenz (derselbe Lauf): circa 247.725 Mikrosekunden.
libc memcmp (derselbe Lauf): circa 3.882 Mikrosekunden.
Diese Daten demonstrieren, dass SIMD-Featureerkennung keine
Garantie fuer einen schnelleren Spezialvergleich ist.
Das bereits plattformspezifisch optimierte libc memcmp
wird daher fuer die eigentlichen Fast-Cache-Gleichheitschecks
verwendet; AVX2/NEON bleiben als separate CPU-Backend-Bausteine.
120.000 Eintraege identisch, 60.000 veraendert.

Das ist nur ein einzelner CPU-Speicherbenchmark, keine
belastbare Aussage ueber Textur-Laden, Datentraegerleistung,
OpenSim-HTTP-Fehler oder Bildrate.

## Vergleichsstart

Bisherige Instanz vollstaendig schliessen.

Normaler Standard:
    cd "/home/jam/Schreibtisch/OGL-Viewer/ogl-firestorm"
    bash ogl/scripts/run-viewer-test.sh

Experimenteller Test:
    OGL_FASTCACHE_SKIP_IDENTICAL=1 bash ogl/scripts/run-viewer-test.sh

Nach Weltstart:
    python3 ogl/scripts/ogl-test-report.py

Ab 128 geprueften Schreibversuchen steht bei aktivierter
Optimierung eine Zeile zu uebersprungenen Cachewrites im
Viewer-Log und Testbericht. Bei nur wenigen Schreibvorgaengen
wird kein Wert ausgegeben. Echte Ladezeiten und FPS mit
identischer Szene auf beiden Testlaeufen vergleichen.

## Grenzen

Die zusaetzlichen Festplattenlesevorgaenge koennen auf
manchen SSDs und bei wenig identischen Eintraegen mehr
kosten als sie sparen. Default bleibt deshalb AUS.

Weder Mesh-Dekodierung noch GPU-Textur-Uploads werden
in dieser Runde pauschal schneller. Keine Cache-Dateien,
Anmeldedaten, Systembibliotheken oder individuellen
Betriebssystempfade werden veraendert. Linux, Windows
und macOS verwenden dieselbe Quellen- und CPU-Fallback-
Architektur. Vollstaendige Fremdplattform-Builds sind
separat abzunehmen.
