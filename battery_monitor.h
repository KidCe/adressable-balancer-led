#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>

#define BATTERY_SAMPLE_INTERVAL_MS       20U
#define BATTERY_LOW_MV                 3400U
#define BATTERY_DEFAULT_FULL_MV        4170U
#define BATTERY_FULL_MIN_MV            3500U
#define BATTERY_FULL_MAX_MV            5000U
#define BATTERY_CALIBRATION_OFFSET_MV    30U

typedef struct {
    uint16_t millivolts;
    uint8_t low_due;
    uint8_t recovered;
    uint8_t emergency;
} BatteryStatus;

/* Configure measurement hardware, seed the filter, and start monitoring. */
void battery_monitor_init(void);

/* Advance monitoring time. Safe to call before initialization. */
void battery_monitor_tick(uint32_t elapsed_ms);

/* Enable low-voltage qualification. Disabling clears accumulated low time. */
void battery_monitor_set_low_tracking(uint8_t enabled);

/* Return a consistent snapshot of observable battery state. */
BatteryStatus battery_monitor_status(void);

/* Clear a latched emergency after the recovered flag has been observed. */
void battery_monitor_resume_after_recovery(void);

#endif
