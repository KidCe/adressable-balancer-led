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

## Hardware and connections

- MCU: Puya PY32F002Bx5 (Cortex-M0+)
- Onboard LEDs: 14 addressable RGB LEDs (dual-sided lighting)
- LED data pin: PA7 / SPI1
- Buttons: two pads are present on the PCB (PA6 and PA5). The shipped PCB currently uses a single working button — the second pad was manually disabled on this hardware revision.
- Battery measurement: internal VREFINT via ADC

Power and external LEDs

- The board is designed to run directly from a single-cell (1S) LiPo. In typical racing use the additional current drawn by the LEDs is imperceptible and effectively negligible compared to the motors. Example calculation:

	- 6S 1500 mAh race pack ≈ 25.2 V nominal → 1.5 Ah → energy ≈ 37.8 Wh.
	- If that pack is emptied in ≈90 s (0.025 h), the average flight power is ≈ 37.8 Wh / 0.025 h ≈ 1,512 W.
	- Fourteen WS2812-type LEDs at full white draw ≈ 60 mA each at ≈5 V → ≈4 W total.
	- LED draw ≈ 4 W / 1,512 W ≈ 0.25% of flight power — imperceptible in flight and effectively negligible.

- External addressable LEDs may be attached to the three solder pads (data, +V, GND). The external LEDs run from the same 1S voltage as the onboard LEDs (no separate external supply is required for small chains).

Control and buttons

- All user features are designed to be accessible from a single button when needed. The second button is present on the schematic but may be disconnected on some boards; features remain reachable via the primary control.
- Button lock prevents accidental changes from vibration during flight — after an idle timeout the controls lock and must be explicitly unlocked (three quick clicks) before accepting further input.

Color profiles

- The color profiles follow the common FAI / Betaflight assignments: R1, R2, F2, F4, R7, R8, plus two additional colors for low-band 6 and 7. These cover the standard competition and band needs.

Flashing and programming

- Programming uses a 1.27 mm pogo-pin connector. Four pins (SWCLK, SWDIO, GND, VCC) are the minimum for SWD programming; a 7-pin connector is useful if you want reset and UART (RX/TX) available for future features such as a UART bootloader or Betaflight passthrough.
- Common tools (J-Link OB, ST-Link) work for flashing; a low-cost J-Link OB V8 with USB‑C has been used successfully for testing.

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

The release script performs a clean project build, checks the Keil log for errors, creates AXF/HEX/BIN artifacts, and writes SHA-256 checksums under `release/v1.1.0/`.

## Publishing the User Guide

`USER_GUIDE.html` and `assets/logo.png` are the canonical User Guide inputs. Publish the User Guide in `docs/` with:

```powershell
.\publish_user_guide.ps1
```

Verify that the published User Guide is current without changing files:

```powershell
.\publish_user_guide.ps1 -Check
```

The release build runs this verification and stops if the published User Guide is stale.

## Flashing

The current supported development method is SWD with J-Link through Keil µVision. Select the `PY32F002Bx5` target, build, flash, and verify.

From VS Code:

1. Press `Ctrl+Shift+B` to build the firmware.
2. Open **Terminal → Run Task** and choose **Firmware: Build and Flash** to build and program the connected board.
3. Use **Firmware: Flash** when the current build is already up to date.

The VS Code tasks locate Keil automatically in its usual Windows install locations. If Keil is installed elsewhere, set the `KEIL_UV4` environment variable to the full path of `UV4.exe` before starting VS Code.

Do not disconnect power during settings-sector erase/program operations or firmware flashing. SWD remains the recovery path if an experimental build becomes unresponsive.

## Repository files

- `main.c` — release firmware
- `led_output.c/.h` — WS2812/SPI output module
- `lighting.c/.h` — pure color and frame-rendering module
- `battery_monitor.c/.h` — ADC measurement and battery-protection state module
- `USER_GUIDE.html` — end-user guide and simulator
- `CHANGELOG.md` — public release history
- `HISTORY.md` — detailed internal development notes
- `RELEASE_CHECKLIST.md` — build, hardware-test, and publishing checks
- `AdressableBalancerLEDPY32F002b.uvprojx` — Keil project
- `build_release.ps1` — reproducible release packaging
- `publish_user_guide.ps1` — User Guide publishing and drift verification

## Release status

Version `1.1.0` is the current firmware release. Hardware verification should be completed on the intended PCB revision before distributing binaries broadly.

## Creator

Created by **KidCe** with AI assistance. Follow [@kidce.fpv](https://www.instagram.com/kidce.fpv/) on Instagram.

## License

No open-source license has been selected yet. Until a license file is added, normal copyright restrictions apply. Choose and add a license before inviting third-party code contributions.
