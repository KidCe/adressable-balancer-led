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
- Standalone interactive [user guide](USER_GUIDE.html)

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

See [USER_GUIDE.html](USER_GUIDE.html) for diagrams, detailed behavior, and the interactive simulator.

## Hardware

- MCU: Puya PY32F002Bx5, Cortex-M0+, 24 MHz
- Flash: 24 KB
- SRAM: 3 KB
- Onboard LEDs: 14 addressable RGB LEDs
- LED data: PA7 / SPI1
- Buttons: PA6 and PA5, active low with internal pull-ups
- Battery measurement: internal VREFINT through ADC

The firmware transmits 64 LED positions. Positions 1–14 are the onboard pattern; positions 15–64 repeat that complete pattern. External LEDs require a suitable external supply and a shared ground. Do not power a large external strip through an undersized board supply.

## Flash layout

| Address range | Purpose |
|---|---|
| `0x08000000–0x08004FFF` | Application, maximum 20 KB |
| `0x08005000–0x08005FFF` | Settings journal, 4 KB |

The Keil project limits application linking to `0x5000` bytes so code cannot overlap settings.

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
