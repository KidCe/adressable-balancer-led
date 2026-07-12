# Release checklist

Use this checklist before publishing firmware binaries or creating a GitHub release.

## Build

- [ ] Set the intended version in `main.c`, `CHANGELOG.md`, and the release command.
- [ ] Run `./build_release.ps1 -Version 1.0.0` in PowerShell.
- [ ] Confirm the Keil build reports 0 errors and 0 warnings.
- [ ] Confirm AXF, HEX, BIN, documentation, and `SHA256SUMS.txt` exist in the release folder.

## Hardware regression test

- [ ] Flash a clean board through SWD and verify startup battery indication.
- [ ] Select every normal color in both directions.
- [ ] Power-cycle after a color has remained selected for one second; verify it is restored.
- [ ] Verify the controls lock after inactivity and unlock with the protected triple-click.
- [ ] Enter and leave armed standby; check the fade and heartbeat indication.
- [ ] Enter battery checker mode from power-up; test its brightness boost.
- [ ] Test 5-second full-voltage calibration and 10-second default reset.
- [ ] Check low and critical voltage behavior with a current-limited bench supply.
- [ ] Exercise all hidden lighting modes and their cycling gesture.
- [ ] Connect an external LED chain and verify data through position 64.
- [ ] Confirm SWD can still erase and recover the board.

## Publish

- [ ] Update the dated release section in `CHANGELOG.md`.
- [ ] Review `README.md` and `USER_GUIDE.html` for behavior changes.
- [ ] Choose and add a `LICENSE` before accepting outside contributions.
- [ ] Create an annotated Git tag such as `v1.0.0`.
- [ ] Upload the generated release files and publish concise release notes.
