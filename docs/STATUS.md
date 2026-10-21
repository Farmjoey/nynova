# Funktionsstatus v0.1.0

## Verificeret (på PC, uden hardware)
* Pure logik (gestus, hold-tracker, touch-kalibrering, sikkerhedsregler, demo-data, "--"-formatering): **2418 checks, 0 fejl** (`tools/run_host_tests.sh`, C++11).
* Alle `.cpp`-filer syntakstjekket (`g++ -fsyntax-only`) mod midlertidige stub-headere for Arduino/TFT_eSPI/NimBLE.
  Stubs afspejler *mine* antagelser om bibliotekernes API – det er **ikke** en rigtig kompilering.

## IKKE verificeret
* Kompilering med PlatformIO, linkning, firmwarestørrelse og versionslåste biblioteker (kræver netværk – gøres af GitHub Actions).
* Opstart, display, farver/inversion, touch, kalibrering, lysstyrke, hastighed/flicker på dit board.
* BLE-scanning (NimBLE) og RAM-forbrug under scanning.
* Alt vedr. E-Wheels e2s Urban Max: **ingen forbindelse, ingen protokol, ingen telemetri**.

## Funktioner
| Funktion | Status |
|---|---|
| Dashboard (hastighed, batteri, spænding, effekt, trip/total, mode/fejl) | Implementeret; viser kun demo-data (mærket DEMO) eller `--` |
| Klokkeslæt | Ikke tilgængeligt (ingen RTC/tidskilde) – viser `--:--` |
| BLE | Kun passiv scanning/liste; **ingen forbindelse**, ingen skrivning |
| Scooter-protokol | Ikke understøttet (`kVerified=false`, `kWriteSupported=false`) |
| 20 km/t | Lokal sikkerhedsskærm + hold-for-at-aktivere; kun demo-simulation; viser aldrig "bekræftet" uden verificeret protokol og kvittering |
| Musik (iPhone) | Ikke implementeret (planlagt AMS); knapper inaktive |
| Beskeder | Ikke implementeret (planlagt ANCS); intet gemmes |
| Indstillinger gemmes | Implementeret (NVS) – ikke testet på hardware |
