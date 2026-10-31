# OGL Firestorm: Verbindliche Modul- und Plattformarchitektur

Stand: 31.10.2026, Branch ogl-develop.

## Verbindlicher Grundsatz fuer alle kommenden Arbeiten

Jedes Feature muss fuer Linux, Windows und macOS geplant, gebaut
und getestet werden. Ein Erfolg auf dem Linux-Mint-Entwicklungs-PC
allein reicht NICHT fuer die Plattformfreigabe.

Es gelten gemeinsame APIs, Datentypen und portable Dateien.
Keine absoluten Entwicklerpfade, fest codierten Bildschirmgroessen,
Hersteller-IDs, Shell-Skripte als Runtime-Voraussetzung oder Linux-
only Bibliotheken im gemeinsamen Viewer. Jedes OS bekommt einen
eigenen klar eingegrenzten Adapter, falls noetig.

## Zwei strikt getrennte Erweiterungstypen

### A. Themes / Designs / Optik

- Deklarative, plattformunabhaengige ZIP-Pakete mit Endung .ogltheme:
  theme.json mit versioniertem Schema, IDs, Versionen, Farb-Tokens,
  optionalen PNG- oder WebP-Dateien unter assets/.
- Keine Skripte, nativen Bibliotheken, beliebigen XUI/XML-Overrides
  oder Austauschen von Anmelde-, Passwort- und Sicherheitsdialogen.
- Keine Bearbeitung/Ersetzung der originalen Firestorm-Quelldateien.
- Ein Theme gilt auf Linux, Windows und macOS gleich; unterschiedliche
  DPI, Bildschirmgroessen, Sprachen und dunkle/helle Oberflaechen
  muessen beruecksichtigt werden.
- Installation, Vorschau, Aktivierung, Deaktivierung, Update,
  Rollback und Loeschung sollen spaeter ueber den Viewer-Manager
  erfolgen, mit getrennten benutzerspezifischen Appdaten-Pfaden.
- Der gegenwaertige Theme-Validator prueft nur Schema und Archiv-
  Sicherheit. Ein Live-Installer oder Theme-Switch ist NOCH NICHT gebaut.

### B. Funktionsmodule / Plugins

- Versioniertes, plattformunabhaengiges C-ABI und dokumentierte
  Berechtigungen; keine compilerabhaengigen C++-Objekte ueber ABI.
- Gebuendelte Module muessen pro Plattform/CPU gebaut werden:
  Linux .so, Windows .dll, macOS .dylib.
- Fremder nativer Code darf nicht einfach automatisch als
  Viewer-Erweiterung geladen werden: DLLs haben volle Prozessrechte!
- Externe Plugins sollen zunaechst sandboxed ausgefuehrt werden
  (z.B. separater Prozess oder WebAssembly mit eingeschraenkten
  Rechten). Signierte Pakete allein sind keine Sandbox.
- Rechte granular gestalten: ui.theme, ui.panel, chat.read,
  chat.send, world.read, graphics.render; kein Zugriff auf SSO,
  Passwoerter, Tokens, Session-Cookies oder ganze Dateisysteme.
- Versionierung, API-Kompatibilitaet, GUI-Threadgrenzen,
  Crash-Isolierung, Abschalten/Safe-Mode und Wiederherstellung.
- Die vorhandene OGL-RTX-Erweiterung ist ein gebuendeltes
  Entwickler-Modul; der Modul-Probe ist nur ein ABI-Test, NICHT
  der zukuenftige Plugin-Installer.

## Plattformkompatibilitaet und GPU-Fallback

Linux: x64 und spaeter separat ARM64 pruefen; OpenGL mit
optionalem Vulkan-Raytracing falls Hardware-Features vorhanden.

Windows: x64 und spaeter separat ARM64 pruefen; OpenGL mit
optionalem Vulkan-Raytracing bei passenden GPU-Funktionen.

macOS: Intel und Apple Silicon; OpenGL bzw. spaeter geeigneter
Mac-Rendering-Backend. Vulkan-RTX ist nicht automatisch verfuegbar;
ein GPU-unabhaengiger Fallback ist zwingend. Einen echten
Mac-Raytracing-Backend separat entwickeln und abnehmen.

Keine Features exklusiv fuer eine RTX 4070 SUPER, Linux Mint
oder eine einzelne Aufloesung. RTX bleibt optional.

## Freigabekriterien

1. Statische Sicherheits- und Vertragstests fuer alle Plattformen.
2. Portable C-ABI-CTest-Builds unter Linux, Windows, macOS Intel
   und macOS Apple Silicon in GitHub Actions.
3. Vollstaendige Viewer-Builds und reale GUI-/OpenSim-Abnahmetests
   je Betriebssystem VOR einer allgemeinen Produktfreigabe.
4. Pakete und Updates je OS/CPU; keine Ueberschreibung von
   persoenlichen Einstellungen oder Cache.
5. GPU-, Renderer-, Netzwerk- und Login-Fallbacks pruefen.
6. Windows-Code-Signing, Apple-Signierung, Hardened Runtime,
   Notarisierung und Gatekeeper einschliesslich Plugins pruefen.
7. Eine gruene Linux-CI bedeutet nur Linux-geprueft. CI-Konfiguration
   bedeutet noch nicht, dass fremde OS-Runner erfolgreich liefen.
8. Originale Firestorm-Lizenzen, Attribution und Third-Party-
   Notices erhalten; Plugin-Lizenzen auf Kompatibilitaet pruefen.

Offizielle Referenzen:
https://docs.github.com/en/actions/reference/runners/github-hosted-runners
https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution

## Stand dieser Runde (nur belegte Arbeit)

- Portable C-ABI-Probe unter Windows durch sichere UTF-8/Unicode-
  Pfadkonvertierung, LoadLibraryExW, GetProcAddress, FreeLibrary;
  unter Linux/macOS durch dlopen/dlsym/dlclose.
- Derselbe Modul-Handshake-Test wird fuer alle OS gebaut.
- GitHub-Actions-OGL-Teiltestmatrix hat Linux x64, Windows x64,
  macOS Intel und macOS Apple Silicon.
- ZIP-Theme-Paketvalidator und portables Beispiel vorhanden,
  mit Pfadtraversal-, Symlink-, Datei-/ZIP-Groessen-, Farb- und
  Formatpruefungen sowie einer strikten Liste erlaubter Assets.
- Noch KEIN universeller Plugin-Manager, kein Themeswitch im Viewer,
  keine allgemeine Installation unsignierter Erweiterungen.
- Vollstaendige Windows- und macOS-OGL-Viewer-Builds und
  grafische Abnahmen noch NICHT bestanden/belegt.

## Naechste Phasen

1. Gemeinsame Modul-Registry und Theme-Tokens zur Laufzeit.
2. Sicherer Theme-Installer mit Preview und Ruecksetzen.
3. UI-Einstellungen fuer installierte Themes und berechtigte Plugins.
4. Isolierte Plugin-Runtime mit beschraenktem Capability-API.
5. Releases, Signierung, Packaging, Auto-Update und Abnahme
   auf Linux, Windows und macOS.
