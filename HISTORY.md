Project history — compact summary

Date: 2026-07-11

Timing test result and normal build restored
- Hardware measurement was 10.4 seconds for a nominal 10-second fade, a small difference not worth applying globally.
- Disabled `TIMING_CALIBRATION_TEST` and restored normal product operation.
- Slowed only the armed heartbeat cycle from 1.0 seconds to 1.1 seconds; the double-pulse spacing remains unchanged.

Continuous timing calibration
- The three red preparation flashes now occur only once after startup.
- Nominal 10-second green fades now repeat continuously with no dark pause or preparation flashes between cycles.
- Multiple full-green restart intervals can be timed together and averaged for improved accuracy.

Temporary timing calibration build
- Enabled `TIMING_CALIBRATION_TEST` to bypass normal operation after peripheral initialization.
- The test gives three red ready flashes, then fades a full mirrored green bar to black over a nominal 10 seconds.
- A 2-second dark pause separates repeated measurements.
- Measure from full-green appearance to completely black; disable the test after hardware timing is characterized.

More practical guide structure
- Added short benefit-led introductions to button lock, armed standby, calibration, battery checker, and optional external LED sections.
- Kept direct everyday-use and troubleshooting sections concise to avoid repetitive explanations.

Guide simplification and heartbeat speed
- Slowed the armed heartbeat from approximately 72 BPM to 60 BPM using a 1-second cycle; the second pulse still begins 300 ms after the first.
- Updated the HTML simulator to the same heartbeat timing.
- The simulator now opens after one top-button press instead of requiring three rapid presses.
- Simplified user-guide wording by removing unnecessary numerical brightness and BPM details.

Website synchronized with current firmware
- Documented delayed one-second color persistence, armed low-power standby, 72 BPM heartbeat, checker hold-to-boost, locked-button blackout feedback, and 64-position external LED output.
- Updated the hidden simulator with delayed color saving, standby fade/readiness/heartbeat/wake behavior, lower checker brightness, brightness boost, and lock feedback.
- Kept special-mode activation and operation details undisclosed in the public guide.

Human heartbeat timing
- Tuned the armed indicator to approximately 72 BPM with an 830 ms complete cycle.
- Each pulse lasts 50 ms and the second pulse begins 300 ms after the first, followed by approximately 480 ms before the next heartbeat.

Heartbeat and lock feedback tuning
- Increased the gap between the two armed heartbeat pulses from 80 ms to 180 ms while retaining the 1.5-second cycle.
- When controls are idle-locked and a button attempt is detected, one mirrored LED per PCB side briefly goes black against the selected color.
- Failed unlock attempts restore the selected color; successful triple-click unlocks retain the green confirmation.

Armed heartbeat indicator
- Replaced the single armed-standby pulse with two dim 40 ms pulses separated by 80 ms.
- The heartbeat pattern repeats every 1.5 seconds on one mirrored onboard LED per side.

Armed indicator timing
- Armed low-power standby now pulses every 1.5 seconds instead of every 5 seconds.
- One mirrored onboard LED per PCB side now pulses; external LEDs remain off.

Armed low-power standby
- In normal operation, holding either color button now fades the selected color smoothly to black over 2 seconds.
- Releasing after the fade but before the 5-second calibration threshold plays a subtle two-step mirrored confirmation and enters armed standby.
- Continuing to hold still performs the existing 5-second calibration and 10-second default reset.
- Armed standby keeps all external LEDs off and pulses only onboard LED 1 in the selected color at `2/255` brightness for 30 ms every 5 seconds.
- Pressing either button wakes directly to the selected color at normal brightness.

Lower-power battery checker display
- Reduced normal checker bargraph brightness from about 10% (`26/255`) to about 2% (`5/255`).
- Near the low threshold, checker brightness can fall to `1/255` for additional power savings.
- Holding either button in checker mode now ramps the live bargraph from its low-power level toward full brightness over 3 seconds.
- Releasing the button immediately returns the bargraph to its low-power brightness.
- The automatic low-voltage warning still ramps toward full brightness over 2 minutes.

Extended LED data output
- Increased the transmitted WS2812 stream from 14 to 64 LEDs without increasing the LED framebuffer.
- The 14 onboard LEDs remain the authoritative 7+7 mirrored main-light pattern.
- Output positions 15-64 repeat the complete 14-LED pattern, extending every existing static color, warning, bargraph, and animation to external LEDs.
- External LED power is not supplied or managed by the firmware and must be sized appropriately by the user.

Guide credit
- Added a discreet footer credit for KidCe, noting AI assistance and linking to `@kidce.fpv` on Instagram.

Pre-simulation battery diagram
- The HTML guide now explicitly renders the original two-red, three-amber, two-green battery diagram until the hidden simulator is activated.
- The same seven blocks transfer to live simulator control only after the activation gesture.

Simulator discovery and mobile controls
- The hidden HTML simulator now requires three fast clicks with no more than 250 ms between adjacent clicks.
- Ordinary first and second clicks leave the documentation unchanged.
- Once discovered, the two top buttons move down beside the Battery bar display, keeping controls and output together on mobile screens.
- The simulated startup begins automatically after the activation gesture.

Hidden simulator presentation
- Restored the top of `USER_GUIDE.html` to a documentation-first layout with only the two product buttons visible.
- The existing Battery bar diagram lower on the page is now the single live simulation display.
- Simulation status, voltage, and power controls stay hidden until a top button is pressed.

Simulator fixes
- Corrected the interactive battery bar's CSS color syntax so red, amber, and green appear during startup.
- Fixed simulator gesture recognition to measure time between adjacent clicks rather than requiring the complete sequence to fit inside one timing window.
- The simulated 10-click hidden trigger, three-click mode cycling, and idle-lock unlock gesture now use consecutive click intervals.

Interactive user-guide simulator
- Converted the top of `USER_GUIDE.html` into an interactive product simulation using the seven battery-bar blocks as mirrored LED pairs.
- Added simulated power, adjustable battery voltage, startup sweep/bar, color selection and persistence, calibration holds, idle lock/unlock, and battery-checker behavior.
- Included the undisclosed special behavior in the simulation without documenting its activation sequence.

User documentation
- Added `USER_GUIDE.html`, a standalone responsive quick-start manual with visual button, battery, calibration, lock, and checker-mode diagrams.
- Included desktop, mobile, and print styling with no external dependencies.
- Kept the disco easter egg undocumented, with only a subtle discovery hint.

Third disco mode and cycling
- Added a synchronized full-white strobe on both PCB sides (60 ms on / 60 ms off).
- The armed rapid triple-click gesture now cycles rainbow -> complementary random flash -> white strobe -> rainbow.
- Every mode change re-applies an all-buttons-released arming period before another cycle command is accepted; this cooldown is now 300 ms.

Idle input lock
- Normal color mode locks button input after 10 seconds without button activity.
- While locked, color changes, calibration holds, and disco entry clicks are ignored.
- Three clicks unlock input when consecutive click onsets are 75-250 ms apart.
- A single short green confirmation blink indicates a successful unlock.
- Battery checker and disco operating modes are unaffected.

Mirrored rainbow
- Rainbow disco mode now generates its gradient across seven positions and mirrors it onto the opposite PCB side.

Disco input safety
- Rapid-click timing is now 150 ms for both disco triggers.
- Rainbow mode ignores further clicks until both buttons have remained released for a full second.
- Switching from rainbow to random-flash mode now requires three rapid clicks instead of two.

Disco tuning
- Rapid-click sequences now require gaps below 250 ms (configured at 240 ms).
- Rainbow rotation now completes in 1 second.
- Rainbow mode uses an 8-unit hue step between adjacent LEDs for a stronger strip gradient.
- Random-flash mode uses one shared color on LEDs 0-6 and its 180-degree counter hue on LEDs 7-13.

Disco timing and side-color update
- Reduced the full rainbow rotation time from 1.5 seconds to 1 second.
- Tightened the rapid-click gap from 400 ms to 240 ms (below 250 ms).
- All seven LEDs on one PCB side now share one color in both disco modes.
- The seven LEDs on the opposite side use the complementary color, shifted halfway around the hue wheel.

Disco easter egg
- Ten rapid short clicks on either color button enter rainbow disco mode; the maximum gap between clicks is 400 ms.
- Rainbow mode applies a small hue offset along the strip and rotates through a full color wheel every 1.5 seconds.
- Two additional rapid clicks in rainbow mode switch to a second disco mode with independently randomized LED colors and short dark gaps.
- Both disco modes remain active until the device is power-cycled.
- Long button holds still perform battery-full calibration/reset and do not count as disco clicks.
- Rebuilt successfully with 0 errors and 0 warnings.

Bargraph rounding consistency
- Changed the normal boot battery bargraph to round voltage bands up like checker mode.
- The seventh LED now represents the highest voltage band and no longer requires the measured voltage to exactly equal the calibrated full value.

Battery checker startup display fix
- Checker entry no longer blanks the LEDs while waiting for the startup hold.
- The normal white sweep and initial battery indication always run first.
- If a button was held at power-up, the normal battery bargraph remains visible and refreshes while waiting.
- Releasing the button before the 2-second entry period continues into normal operation.
- Holding through the entry period switches to the persistent 10%-brightness checker bargraph.
- Rebuilt successfully with 0 errors and 0 warnings.

Battery checker mode
- Holding either button continuously for 2 seconds during power-up enters a latched battery checker mode.
- The mode bypasses the normal boot animation, color profile, and normal low-voltage sleep behavior.
- Battery voltage is continuously displayed as a mirrored bargraph at about 10% LED brightness.
- Between 3.45 V and 3.40 V the bargraph progressively dims.
- At or below 3.40 V, the display changes to a red 200 ms on / 800 ms off warning blink.
- Warning brightness starts at 10% and ramps linearly to full brightness over 2 minutes.
- If voltage recovers above 3.40 V, the bargraph returns and the warning ramp resets.
- Rebuilt successfully with 0 errors and 0 warnings.

Calibration reset update
- Continuing to hold either button for 10 seconds restores the default battery-full level of 4.17 V.
- The normal 5-second calibration remains active and shows two confirmation blinks.
- A 10-second reset shows three confirmation blinks and does not change the selected color.

Field calibration update
- Holding either color button for 5 seconds saves the currently measured supply voltage as the battery-full level.
- A successful calibration is acknowledged by two short green blinks on the outer LED pair.
- A long hold does not change the selected color; short presses now change color when the button is released.
- The calibrated level is stored together with the selected color in the existing flash journal.
- Boot battery bar scaling and the battery-full blink threshold now use the calibrated value.
- Accepted calibration range is 3.5 V to 5.0 V; the default remains 4.17 V.
- Rebuilt successfully with 0 errors and 0 warnings.

PY32F002B flash reset fix
- Confirmed that enabling the old halfword save caused a reboot immediately after a button press.
- Replaced STM32-style halfword programming with the PY32F002B native flash sequence from the installed Puya device pack.
- Settings now use complete 128-byte programming pages in the final 4 KB flash sector (`0x08005000-0x08005FFF`).
- The journal holds 32 color selections before erasing and restarting the settings sector.
- Reduced the Keil application IROM region to `0x5000` so code can never overlap the settings sector.
- Rebuilt successfully with 0 errors and 0 warnings; hardware persistence test is pending.

Latest persistence update
- Re-enabled runtime color saving (`RUNTIME_SAVE_ENABLED = 1`).
- Each confirmed button color change is appended to the flash journal and restored during the next boot.
- Added journal rollover after 511 saved changes.
- Fixed nested interrupt handling around flash erase/program operations by preserving PRIMASK.
- Reserved `0x08005C00-0x08005FFF` for settings by reducing the Keil application IROM size to `0x5C00`.
- Current build still needs to be flashed and verified on the device.

Summary
- Goal: fix build/flash integration and add persistent LED profile storage; debug unexpected reboot on button press.

What I changed
- Implemented flash-backed persistent mode storage in `main.c` (signature + halfword entries).
- Added flash helper functions: `flash_unlock`, `flash_lock`, `flash_erase_storage_page`, `flash_program_halfword`, `flash_wait_ready`.
- Aligned flash access to device headers; switched storage pointers to `uint16_t *`.
- Added IRQ guards around flash register operations to protect critical sections.
- Implemented `RUNTIME_SAVE_ENABLED` compile-time switch and wrapped runtime `save_mode()` calls to allow disabling runtime flash writes.
- The earlier build skipped saving when the journal was full; the latest update now erases and restarts the journal at rollover.
- Added WS2812 over SPI helpers (`ws_send_byte`, `ws_send_color`, `ws_show`) and an `WS_ENABLED` flag to compile out LED transmission for bisecting.
- Adjusted SysTick-based sleep (`sleep_ms_wfi`) and debouncing logic; added LED boot/battery display flows.

Build & test status
- Builds: Keil/ArmClang builds succeed (AXF produced). Warnings present (macro redefinitions vs device headers and minor pointer/inlined-function warnings) but no errors.
- Flash: AXF can be flashed via UV4/JLink (erase/program/verify OK).
- Earlier runtime test: device rebooted on button press with runtime flash writes disabled (`RUNTIME_SAVE_ENABLED = 0`).
- WS2812: Temporarily compiled out and then re-enabled; LEDs were not lit after one flash (I re-enabled and rebuilt for feedback).

Known issues & hypotheses
- Reboot persists with runtime flash writes disabled — flash writes unlikely root cause.
- Candidate causes: SPI/WS2812 timing or IRQ interactions, SysTick/WFI sleep interaction with button IRQs, button debounce logic, stack/interrupt corruption.

Next recommended bisect steps
1. Disable `__WFI()`/sleep path and SysTick-driven wait to see if reboot stops.
2. If reboot persists, isolate SPI: keep SPI initialized but skip `ws_send_*` calls (already have `WS_ENABLED` flag); test with SPI disabled entirely.
3. Instrument button handling (toggle spare GPIO before/after critical sections) to narrow exact crash point.
4. If suspect flash, implement run-from-RAM flash routines or perform erase-only at boot.

Files changed (main ones)
- `main.c` — all major changes (flash, storage, WS2812, sleep, button handling)

Notes for pickup
- To test quickly: build then flash `Objects/AdressableBalancerLEDPY32F002b.axf` via Keil UV4; verify LEDs show state and press buttons.
- If reboot occurs, tell me whether LEDs change briefly before reboot and whether button holds vs presses behave differently.

If you want, I can now implement step 1 (disable WFI sleep path) and rebuild for the next test.
Release preparation (v1.0.0)
- Removed the temporary 10-second timing calibration display.
- Added bounded waits and safe fallbacks for SPI, ADC, and flash operations.
- Added verification after settings-sector erase and programming.
- Reduced normal-operation battery sampling to once every 250 ms.
- Preserved the previous interrupt state around both WS2812 frame senders.
- Added firmware version metadata and centralized 24 MHz timing constants.
- Enabled HEX generation in the Keil project.
- Added README, public changelog, gitignore, and automated release packaging.
- Synced the explicitly requested HTML guide simulator with the final heartbeat timing.
- Final post-cleanup Keil build and hardware regression test are still required.
