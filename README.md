# Addressable Balancer LED

Firmware and user documentation for a compact PY32F002B-based addressable LED controller designed for FPV builds.

The board drives 14 onboard WS2812-compatible LEDs as a mirrored 7+7 main light, monitors its supply voltage, remembers the selected color, and provides practical battery and standby modes through two buttons.

## Highlights

- Eight selectable color profiles
- Mirrored 7+7 onboard lighting
- Last-color memory with delayed, wear-reduced flash saving
- Calibrated startup battery bargraph
- Low and critical battery warnings
- Field calibration without a programmer
- Persistent battery checker mode
- Armed low-power standby for connecting a battery before use
- Automatic button lock to avoid changes caused by vibration
- Data output extended to 64 LED positions
- Standalone interactive [user guide](https://kidce.github.io/adressable-balancer-led/USER_GUIDE.html)

## Basic operation

| Action | Result |
|---|---|
| Tap Button 1 | Next color |
| Tap Button 2 | Previous color |
| Leave a color selected for 1 second | Save it for the next power-up |
| Triple-click after controls lock | Unlock controls |
| Hold either button for 2 seconds, then release before 5 seconds | Enter armed standby |
| Hold either button for 5 seconds | Save the present voltage as full |
| Continue holding for 10 seconds | Restore the default 4.17 V full level |
| Hold a button from power-up through startup and for 2 more seconds | Enter battery checker mode |

See the live [interactive user guide](https://kidce.github.io/adressable-balancer-led/USER_GUIDE.html) for diagrams, detailed behavior, and the interactive simulator.

## Hardware

## Hardware and connections

- MCU: Puya PY32F002Bx5 (Cortex-M0+)
- Onboard LEDs: 14 addressable RGB LEDs (implemented as mirrored pairs)
- LED data pin: PA7 / SPI1
- Buttons: two pads are present on the PCB (PA6 and PA5). The shipped PCB currently uses a single working button — the second pad was manually disabled on this hardware revision.
- Battery measurement: internal VREFINT via ADC

Power and external LEDs

- The board is designed to run directly from a single-cell (1S) LiPo. For normal racing use this is safe because the motors draw orders of magnitude more current than the LEDs, so LED current does not significantly affect battery voltage during flight.
- External addressable LEDs can be connected to the three solder pads labeled for data, power, and ground. The board provides the data signal; external LEDs should be powered from a suitably sized supply and share ground with the board. No separate data-level shifting or external supply is required for small chains, but large LED strips must have their own power.
- The firmware outputs up to 64 LED positions: onboard LEDs occupy the first 14 positions and the remainder repeat that pattern.

Control and buttons

- All user features are designed to be accessible from a single button when needed. The second button is present on the schematic but may be disconnected on some boards; features remain reachable via the primary control.
- Button lockup prevents accidental changes from vibration during flight — after an idle timeout the controls lock and must be explicitly unlocked (three quick clicks) before accepting further input.

Color profiles

- The color profiles follow the common FAI / Betaflight assignments: R1, R2, F2, F4, R7, R8, and two additional colors for low-band 6 and 7. These six (plus two extended) profiles cover the standard competition and band needs.

Flashing and programming

- Programming uses a 1.27 mm pogo-pin connector. Minimum required for SWD programming is 4 pins (SWCLK, SWDIO, GND, VCC). A 7-pin connector is recommended if you want reset and UART (RX/TX) available for future features such as a UART bootloader or passthrough via Betaflight.
- I used a low-cost J-Link OB (V8) with USB‑C from marketplaces for flashing; ST-Link and other ARM programmers that support SWD should also work.

Notes

- The README previously listed flash and SRAM sizes; that low-level detail has been removed here because it is not required for everyday use of the board.

## Building

Requirements:

- Keil µVision 5
- Arm Compiler 6
- Puya PY32F0xx Device Family Pack 1.2.8
- ARM CMSIS 6.3.0

Build in µVision or run:

```powershell
.\build_release.ps1
```

The release script performs a clean project build, checks the Keil log for errors, creates AXF/HEX/BIN artifacts, and writes SHA-256 checksums under `release/v1.0.0/`.

## Flashing

The current supported development method is SWD with J-Link through Keil µVision. Select the `PY32F002Bx5` target, build, flash, and verify.

Do not disconnect power during settings-sector erase/program operations or firmware flashing. SWD remains the recovery path if an experimental build becomes unresponsive.

## Repository files

- `main.c` — release firmware
- `USER_GUIDE.html` — end-user guide and simulator
- `CHANGELOG.md` — public release history
- `HISTORY.md` — detailed internal development notes
- `RELEASE_CHECKLIST.md` — build, hardware-test, and publishing checks
- `AdressableBalancerLEDPY32F002b.uvprojx` — Keil project
- `build_release.ps1` — reproducible release packaging

## Release status

Version `1.0.0` is the first major software release. Hardware verification should be completed on the intended PCB revision before distributing binaries broadly.

## Creator

Created by **KidCe** with AI assistance. Follow [@kidce.fpv](https://www.instagram.com/kidce.fpv/) on Instagram.

## License

No open-source license has been selected yet. Until a license file is added, normal copyright restrictions apply. Choose and add a license before inviting third-party code contributions.
