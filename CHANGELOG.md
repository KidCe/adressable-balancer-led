# Changelog

All notable public changes to this project will be documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and releases use [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Planned

- Hardware testing and feedback for the first public release.

## [1.0.0] - 2026-07-12

### Added

- Mirrored 7+7 onboard WS2812 lighting with eight selectable color profiles.
- Persistent color and calibrated full-voltage storage using a wear-reduced flash journal.
- Delayed color commit so rapid browsing saves only the final stable selection.
- White startup sweep, calibrated battery bargraph, and full-battery confirmation.
- Low and critical battery warnings.
- Field calibration with 5-second save and 10-second default reset gestures.
- Battery checker mode with low-power display, hold-to-boost visibility, and progressive empty-battery warning.
- Idle button lock and protected triple-click unlock.
- Armed low-power standby with fade-down, release-ready feedback, and dim heartbeat indicator.
- Extended WS2812 stream for up to 64 positions while retaining the onboard 14-LED pattern as the source.
- Three undisclosed optional lighting effects.
- Responsive standalone HTML user guide with an interactive simulator.

### Safety and reliability

- Reserved a dedicated 4 KB settings sector outside the linked application region.
- Added bounded ADC, SPI, and flash waits.
- Added flash erase/program verification and delayed-save retry behavior.
- Reduced continuous battery polling to a 250 ms interval.
- Centralized timing calculations around the 24 MHz system-clock definition.
