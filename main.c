#include "py32f0xx.h"
#include "battery_monitor.h"
#include "led_output.h"
#include "lighting.h"
#include "platform_irq.h"

/* Firmware identity embedded in the release binary. */
#define FW_VERSION_MAJOR 1
#define FW_VERSION_MINOR 1
#define FW_VERSION_PATCH 0

/* Searchable ASCII identifier retained in release images and crash dumps. */
__attribute__((used)) const char firmware_version[] =
    "AddressableBalancerLED v1.1.0";

/* All delay calculations derive from this single clock definition. */
#define SYSTEM_CLOCK_HZ            24000000UL
#define SYSTICK_TICKS_PER_MS        (SYSTEM_CLOCK_HZ / 1000UL)
#define FLASH_TIMEOUT_LOOPS         1000000UL

// =========================
// Pin / LED config
// =========================

#define BTN1_PIN    6   // PA6
#define BTN2_PIN    5   // PA5

#define IN_PA0_PIN  0
#define IN_PB0_PIN  0

// =========================
// Brightness config
// =========================

#define NORMAL_BRIGHTNESS          255
#define BATTERY_DISPLAY_BRIGHTNESS  64
#define PROFILE_RAMP_START_PERCEIVED 64
#define PROFILE_RAMP_MS            1000UL
#define PROFILE_RAMP_FRAME_MS        20UL

// =========================
// Persistent saved profile storage
// =========================

#define FLASH_STORAGE_SECTOR_START 0x08005000UL
#define FLASH_STORAGE_SECTOR_SIZE  0x1000UL
#define FLASH_PROGRAM_PAGE_SIZE    0x80UL
#define FLASH_STORAGE_SIGNATURE    0xA5A55A5AUL
#define FLASH_STORAGE_EMPTY        0xFFFFFFFFUL
#define FLASH_STORAGE_ENTRY_VALUE(mode) (0xFFFFFF00UL | ((mode) & 0xFFu))
#define FLASH_STORAGE_ENTRY_MODE(entry) ((uint8_t)((entry) & 0xFFu))

/* Save each confirmed color change in the flash journal. */
#define RUNTIME_SAVE_ENABLED 1

#ifndef FLASH_BASE_ADDR
#define FLASH_BASE_ADDR            0x40022000UL
#endif

#ifndef FLASH_AR
#define FLASH_AR                 (*(volatile uint32_t *)(FLASH_BASE_ADDR + 0x14U))
#endif

#ifndef FLASH_KEY1
#define FLASH_KEY1                0x45670123UL
#endif

#ifndef FLASH_KEY2
#define FLASH_KEY2                0xCDEF89ABUL
#endif

#ifndef FLASH_SR_BSY
#define FLASH_SR_BSY              (1U << 16)
#endif

#ifndef FLASH_SR_PGERR
#define FLASH_SR_PGERR            (1U << 2)
#endif

#ifndef FLASH_SR_WRPERR
#define FLASH_SR_WRPERR           (1U << 4)
#endif

#ifndef FLASH_SR_OPTVERR
#define FLASH_SR_OPTVERR          (1U << 15)
#endif

#ifndef FLASH_SR_EOP
#define FLASH_SR_EOP              (1U << 0)
#endif

#ifndef FLASH_CR_LOCK
#define FLASH_CR_LOCK             (1U << 31)
#endif

#ifndef FLASH_CR_PGSTRT
#define FLASH_CR_PGSTRT           (1U << 19)
#endif

#ifndef FLASH_CR_PER
#define FLASH_CR_PER              (1U << 1)
#endif

#ifndef FLASH_CR_SER
#define FLASH_CR_SER              (1U << 11)
#endif

#ifndef FLASH_CR_PG
#define FLASH_CR_PG               (1U << 0)
#endif

static uint8_t load_saved_settings(uint16_t *full_mv);
static uint8_t save_settings(uint8_t mode, uint16_t full_mv);
static void flash_unlock(void);
static void flash_lock(void);
static uint8_t flash_erase_storage_sector(void);
static uint8_t flash_program_page(volatile uint32_t *address, uint8_t mode,
                                  uint16_t full_mv);
static uint8_t flash_wait_ready(void);
static uint32_t *flash_storage_begin(void);
static uint32_t *flash_storage_end(void);
static void sleep_ms_wfi(uint32_t ms);
static void battery_bargraph_show(uint16_t mv);
static void battery_low_power_mode(uint8_t *mode);
static void battery_critical_sleep_mode(void);

// =========================
// Battery config
// =========================

#define BOOT_SWEEP_STEP_MS         50
#define BOOT_SWEEP_WHITE_LEVEL     35

#define BATTERY_DISPLAY_MS         750

#define FULL_BLINK_COUNT           3
#define FULL_BLINK_ON_MS           100
#define FULL_BLINK_OFF_MS          100

#define LOW_POWER_DIM_LEVEL        10
#define LOW_POWER_BLINK_LEVEL      65
#define LOW_POWER_BLINK_ON_MS      100
#define LOW_POWER_BLINK_OFF_MS     900

#define CRITICAL_BLINK_LEVEL       45
#define CRITICAL_BLINK_ON_MS       60
#define CRITICAL_SLEEP_MS          2000

#define CHECKER_ENTRY_HOLD_MS      2000
#define CHECKER_ENTRY_FRAME_MS       50
#define CHECKER_ENTRY_BLINK_MS       80
#define CHECKER_UPDATE_MS          250
#define CHECKER_LOW_MV             3450
#define CHECKER_WARNING_MV         3400
#define CHECKER_BRIGHTNESS         5
#define CHECKER_MIN_BRIGHTNESS     1
#define CHECKER_HOLD_RAMP_MS       3000UL
#define CHECKER_WARNING_RAMP_MS    120000UL
#define CHECKER_BLINK_ON_MS        100
#define CHECKER_BLINK_GAP_MS       100
#define CHECKER_BLINK_OFF_MS       1800

// =========================
// Button config
// =========================

#define DEBOUNCE_MS                20
#define MAIN_LOOP_DELAY_MS         5
#define CALIBRATION_HOLD_MS         5000
#define CALIBRATION_RESET_HOLD_MS   10000
#define CALIBRATION_POLL_MS         10
#define CALIBRATION_BLINK_COUNT     2
#define CALIBRATION_RESET_BLINK_COUNT 3
#define CALIBRATION_BLINK_MS        100

#define DISCO_TRIGGER_CLICKS        10
#define DISCO_SECOND_MODE_CLICKS    3
#define DISCO_CLICK_GAP_MS          150
#define DISCO_SECOND_MODE_ARM_MS    300
#define DISCO_RAINBOW_CYCLE_MS      1000
#define DISCO_RAINBOW_FRAME_MS      20
#define DISCO_RAINBOW_HUE_OFFSET    8
#define DISCO_COUNTER_HUE_OFFSET    128
#define DISCO_RANDOM_ON_MS          100
#define DISCO_RANDOM_OFF_MS         50
#define DISCO_WHITE_ON_MS           60
#define DISCO_WHITE_OFF_MS          60

#define IDLE_LOCK_MS                10000
#define UNLOCK_CLICKS               3
#define UNLOCK_MIN_INTERVAL_MS      75
#define UNLOCK_MAX_INTERVAL_MS      250

#define ARMED_FADE_MS               2000UL
#define ARMED_FADE_STEP_MS          50
#define ARMED_INDICATOR_LEVEL       2
#define ARMED_INDICATOR_ON_MS       50
#define ARMED_HEARTBEAT_GAP_MS      250
#define ARMED_INDICATOR_PERIOD_MS   1100UL
#define ARMED_POLL_MS               10
#define ARMED_ENTRY_LEVEL           8
#define ARMED_ENTRY_PULSE_MS        70
#define ARMED_READY_LEVEL           5
#define ARMED_READY_ON_MS           100
#define ARMED_READY_PERIOD_MS       500
#define MODE_SAVE_DELAY_MS          1000

static LedFrame leds;

static volatile uint32_t systick_sleep_ticks = 0;
static uint16_t battery_full_mv = BATTERY_DEFAULT_FULL_MV;
static uint8_t mode_save_pending = 0;
static uint8_t pending_mode = 0;
static uint32_t pending_mode_ms = 0;

static const Color color_profiles[8] = {
    

    {255,   0,   0},
    {255, 120,   0},
    {255, 255,   0},
    {  0, 255,   0},
    {  0, 255, 255},
    {  0,   0, 255},
    {255,   0, 255},
    {255, 255, 255}
};

// =========================
// Delay via SysTick
// =========================

void SysTick_Handler(void)
{
    systick_sleep_ticks++;
}

static void delay_ms(uint32_t ms)
{
    sleep_ms_wfi(ms);
}

static void sleep_ms_wfi(uint32_t ms)
{
    systick_sleep_ticks = 0;

    SysTick->LOAD = SYSTICK_TICKS_PER_MS - 1U;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;

    while (systick_sleep_ticks < ms) {
        __WFI();

        battery_monitor_tick(1U);
    }

    SysTick->CTRL = 0;
}

// =========================
// Helpers
// =========================

/* Frame helpers keep animation code focused on intent while the lighting
 * module owns frame mutation details. */
static Color color_scaled(Color color, uint8_t brightness)
{
    return lighting_scale_color(color, brightness);
}

static void leds_clear(void)
{
    lighting_clear(leds);
}

static void leds_set_all(uint8_t r, uint8_t g, uint8_t b)
{
    Color color = {r, g, b};
    lighting_fill(leds, color);
}

static void leds_set_mirrored(uint8_t index, Color color)
{
    lighting_set_mirrored(leds, index, color);
}

// =========================
// GPIO
// =========================

static void gpio_init(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;

    GPIOA->MODER &= ~(3U << (BTN1_PIN * 2));
    GPIOA->PUPDR &= ~(3U << (BTN1_PIN * 2));
    GPIOA->PUPDR |=  (1U << (BTN1_PIN * 2));

    GPIOA->MODER &= ~(3U << (BTN2_PIN * 2));
    GPIOA->PUPDR &= ~(3U << (BTN2_PIN * 2));
    GPIOA->PUPDR |=  (1U << (BTN2_PIN * 2));

    GPIOA->MODER &= ~(3U << (IN_PA0_PIN * 2));
    GPIOA->PUPDR &= ~(3U << (IN_PA0_PIN * 2));
    GPIOA->PUPDR |=  (1U << (IN_PA0_PIN * 2));

    GPIOB->MODER &= ~(3U << (IN_PB0_PIN * 2));
    GPIOB->PUPDR &= ~(3U << (IN_PB0_PIN * 2));
    GPIOB->PUPDR |=  (1U << (IN_PB0_PIN * 2));

}

static void ws_show(void)
{
    led_output_show(leds, LED_OUTPUT_REPEAT_FRAME);
}

static void ws_show_armed_frame(void)
{
    led_output_show(leds, LED_OUTPUT_FRAME_THEN_OFF);
}

/* Wait in sampling-sized chunks so long LED pauses cannot hide an emergency
 * or delay a confirmed recovery by hundreds of milliseconds. */
static uint8_t battery_protection_wait(uint32_t ms, uint8_t check_emergency,
                                       uint8_t check_recovery)
{
    while (ms > 0) {
        uint32_t chunk = (ms > BATTERY_SAMPLE_INTERVAL_MS) ?
                         BATTERY_SAMPLE_INTERVAL_MS : ms;
        delay_ms(chunk);
        ms -= chunk;

        BatteryStatus status = battery_monitor_status();
        if (check_emergency && status.emergency)
            return 1;
        if (check_recovery && status.recovered)
            return 2;
    }

    return 0;
}

// =========================
// Boot animation / battery display
// =========================

static void boot_sweep_white(void)
{
    leds_clear();
    ws_show();

    Color white = {BOOT_SWEEP_WHITE_LEVEL,
                   BOOT_SWEEP_WHITE_LEVEL,
                   BOOT_SWEEP_WHITE_LEVEL};

    for (uint8_t i = 0; i < LED_HALF; i++) {
        leds_set_mirrored(i, white);
        ws_show();
        delay_ms(BOOT_SWEEP_STEP_MS);
    }
}

static void battery_not_ready_blink(void)
{
    for (uint8_t i = 0; i < FULL_BLINK_COUNT; i++) {
        leds_set_all(255, 0, 0);
        ws_show();
        delay_ms(FULL_BLINK_ON_MS);

        leds_clear();
        ws_show();
        delay_ms(FULL_BLINK_OFF_MS);
    }
}

static void battery_race_ready_show(void)
{
    uint16_t mv = battery_monitor_status().millivolts;
    battery_bargraph_show(mv);
    if (mv < battery_full_mv)
        battery_not_ready_blink();
}

static void battery_bargraph_render(uint16_t mv)
{
    lighting_render_battery_bar(leds, mv, BATTERY_LOW_MV,
                                battery_full_mv,
                                BATTERY_DISPLAY_BRIGHTNESS);
    ws_show();
}

static void battery_bargraph_show(uint16_t mv)
{
    battery_bargraph_render(mv);
    delay_ms(BATTERY_DISPLAY_MS);
}

static uint8_t checker_bar_brightness(uint16_t mv)
{
    if (mv >= CHECKER_LOW_MV)
        return CHECKER_BRIGHTNESS;
    if (mv <= CHECKER_WARNING_MV)
        return CHECKER_MIN_BRIGHTNESS;

    return (uint8_t)(CHECKER_MIN_BRIGHTNESS +
        ((uint32_t)(mv - CHECKER_WARNING_MV) *
         (CHECKER_BRIGHTNESS - CHECKER_MIN_BRIGHTNESS)) /
        (CHECKER_LOW_MV - CHECKER_WARNING_MV));
}

static void checker_bargraph_show(uint16_t mv, uint8_t brightness)
{
    lighting_render_battery_bar(leds, mv, CHECKER_WARNING_MV,
                                battery_full_mv, brightness);
    ws_show();
}

static uint32_t *flash_storage_begin(void)
{
    return (uint32_t *)FLASH_STORAGE_SECTOR_START;
}

static uint32_t *flash_storage_end(void)
{
    return (uint32_t *)(FLASH_STORAGE_SECTOR_START + FLASH_STORAGE_SECTOR_SIZE);
}

static uint8_t load_saved_settings(uint16_t *full_mv)
{
    uint32_t *page = flash_storage_begin();
    uint32_t *end = flash_storage_end();
    uint8_t last_mode = 0;
    uint16_t last_full_mv = BATTERY_DEFAULT_FULL_MV;
    uint8_t found = 0;

    /* Entries are append-only. Scanning until the first erased page finds the
     * newest valid settings without rewriting old flash cells. */
    while (page < end) {
        uint32_t signature = page[0];
        if (signature == FLASH_STORAGE_EMPTY)
            break;

        if (signature == FLASH_STORAGE_SIGNATURE) {
            uint8_t mode = FLASH_STORAGE_ENTRY_MODE(page[1]);
            if (mode < 8) {
                last_mode = mode;
                uint32_t saved_full_mv = page[2];
                if (saved_full_mv >= BATTERY_FULL_MIN_MV &&
                    saved_full_mv <= BATTERY_FULL_MAX_MV) {
                    last_full_mv = (uint16_t)saved_full_mv;
                }
                found = 1;
            }
        }

        page += FLASH_PROGRAM_PAGE_SIZE / sizeof(uint32_t);
    }

    *full_mv = last_full_mv;
    return found ? last_mode : 0;
}

static uint8_t save_settings(uint8_t mode, uint16_t full_mv)
{
    if (mode >= 8 || full_mv < BATTERY_FULL_MIN_MV ||
        full_mv > BATTERY_FULL_MAX_MV)
        return 0;

    uint16_t current_full_mv;
    uint8_t current_mode = load_saved_settings(&current_full_mv);
    /* Avoid consuming a journal entry when neither setting changed. */
    if (current_mode == mode && current_full_mv == full_mv)
        return 1;

    uint32_t *page = flash_storage_begin();
    uint32_t *end = flash_storage_end();

    while (page < end) {
        if (page[0] == FLASH_STORAGE_EMPTY)
            break;
        page += FLASH_PROGRAM_PAGE_SIZE / sizeof(uint32_t);
    }

    if (page >= end) {
        if (!flash_erase_storage_sector())
            return 0;
        page = flash_storage_begin();
    }

    return flash_program_page(page, mode, full_mv);
}

static void schedule_mode_save(uint8_t mode)
{
    /* Restarting this timer on every click means browsing through colors only
     * writes the profile that remains selected for a full second. */
    pending_mode = mode;
    pending_mode_ms = 0;
    mode_save_pending = 1;
}

static void cancel_mode_save(void)
{
    mode_save_pending = 0;
    pending_mode_ms = 0;
}

static void service_mode_save(uint32_t elapsed_ms)
{
    if (!mode_save_pending)
        return;

    pending_mode_ms += elapsed_ms;
    if (pending_mode_ms >= MODE_SAVE_DELAY_MS) {
#if RUNTIME_SAVE_ENABLED
        if (!save_settings(pending_mode, battery_full_mv)) {
            /* Keep the request pending and retry later without blocking UI. */
            pending_mode_ms = 0;
            return;
        }
#endif
        cancel_mode_save();
    }
}

static uint8_t flash_wait_ready(void)
{
    uint32_t timeout = FLASH_TIMEOUT_LOOPS;
    while (FLASH->SR & FLASH_SR_BSY) {
        if (--timeout == 0)
            return 0;
    }

    uint32_t status = FLASH->SR;
    if (status & (FLASH_SR_PGERR | FLASH_SR_WRPERR | FLASH_SR_OPTVERR))
        return 0;


    FLASH->SR &= ~FLASH_SR_EOP;
    return 1;
}

static void flash_unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static uint8_t flash_erase_storage_sector(void)
{
    uint32_t primask = platform_irq_save_and_disable();
    uint8_t ok;

    flash_unlock();
    FLASH->SR |= FLASH_SR_EOP;
    FLASH->CR |= FLASH_CR_SER;
    *(volatile uint32_t *)FLASH_STORAGE_SECTOR_START = 0xFFFFFFFFUL;
    __DSB();
    ok = flash_wait_ready();
    FLASH->CR &= ~FLASH_CR_SER;
    flash_lock();
    platform_irq_restore(primask);
    if (ok) {
        volatile uint32_t *sector =
            (volatile uint32_t *)FLASH_STORAGE_SECTOR_START;
        ok = (sector[0] == FLASH_STORAGE_EMPTY) &&
             (sector[(FLASH_STORAGE_SECTOR_SIZE / sizeof(uint32_t)) - 1U] ==
              FLASH_STORAGE_EMPTY);
    }
    return ok;
}

static uint8_t flash_program_page(volatile uint32_t *address, uint8_t mode,
                                  uint16_t full_mv)
{
    uint32_t data[FLASH_PROGRAM_PAGE_SIZE / sizeof(uint32_t)];
    const uint32_t word_count = FLASH_PROGRAM_PAGE_SIZE / sizeof(uint32_t);
    uint32_t primask = platform_irq_save_and_disable();
    uint8_t ok;

    for (uint32_t i = 0; i < word_count; i++)
        data[i] = FLASH_STORAGE_EMPTY;
    data[0] = FLASH_STORAGE_SIGNATURE;
    data[1] = FLASH_STORAGE_ENTRY_VALUE(mode);
    data[2] = full_mv;

    flash_unlock();
    FLASH->SR |= FLASH_SR_EOP;
    FLASH->CR |= FLASH_CR_PG;

    for (uint32_t i = 0; i < word_count; i++) {
        address[i] = data[i];
        if (i == (word_count - 2U))
            FLASH->CR |= FLASH_CR_PGSTRT;
    }

    __DSB();
    ok = flash_wait_ready();
    FLASH->CR &= ~FLASH_CR_PG;
    flash_lock();
    platform_irq_restore(primask);
    if (ok) {
        ok = (address[0] == FLASH_STORAGE_SIGNATURE) &&
             (address[1] == FLASH_STORAGE_ENTRY_VALUE(mode)) &&
             (address[2] == full_mv);
    }
    return ok;
}

// =========================
// Buttons
// =========================

static uint8_t btn1_pressed_raw(void)
{
    return ((GPIOA->IDR & (1U << BTN1_PIN)) == 0);
}

static uint8_t btn2_pressed_raw(void)
{
    return ((GPIOA->IDR & (1U << BTN2_PIN)) == 0);
}

static uint8_t button_pressed_event(uint8_t button)
{
    static uint8_t last_btn1 = 0;
    static uint8_t last_btn2 = 0;

    uint8_t current;
    uint8_t *last;

    if (button == 1) {
        current = btn1_pressed_raw();
        last = &last_btn1;
    } else {
        current = btn2_pressed_raw();
        last = &last_btn2;
    }

    if (current && !(*last)) {
        delay_ms(DEBOUNCE_MS);

        if ((button == 1 && btn1_pressed_raw()) ||
            (button == 2 && btn2_pressed_raw())) {
            *last = 1;
            return 1;
        }
    }

    if (!current)
        *last = 0;

    return 0;
}

// =========================
// Color mode
// =========================

static void show_color_profile(uint8_t mode)
{
    Color c = color_scaled(color_profiles[mode], NORMAL_BRIGHTNESS);

    leds_set_all(c.r, c.g, c.b);
    ws_show();
}

static void show_color_profile_ramp(uint8_t mode)
{
    for (uint32_t elapsed_ms = 0; elapsed_ms < PROFILE_RAMP_MS;
         elapsed_ms += PROFILE_RAMP_FRAME_MS) {
        uint8_t perceived_brightness = (uint8_t)(
            PROFILE_RAMP_START_PERCEIVED +
            ((255UL - PROFILE_RAMP_START_PERCEIVED) * elapsed_ms) /
            PROFILE_RAMP_MS);
        Color color = lighting_scale_perceived(color_profiles[mode],
                                               perceived_brightness);

        leds_set_all(color.r, color.g, color.b);
        ws_show();
        delay_ms(PROFILE_RAMP_FRAME_MS);

        if (battery_monitor_status().emergency)
            return;
    }

    show_color_profile(mode);
}

static void show_low_power_profile(uint8_t mode, uint8_t warning_on)
{
    Color c = color_scaled(color_profiles[mode], LOW_POWER_DIM_LEVEL);

    leds_set_all(c.r, c.g, c.b);

    if (warning_on) {
        Color red = {LOW_POWER_BLINK_LEVEL, 0, 0};
        leds_set_mirrored(0, red);
    }

    ws_show();
}

static uint8_t button_pressed_raw(uint8_t button)
{
    return (button == 1) ? btn1_pressed_raw() : btn2_pressed_raw();
}

static void armed_entry_animation(uint8_t mode)
{
    Color c = color_scaled(color_profiles[mode], ARMED_ENTRY_LEVEL);

    for (uint8_t pulse = 0; pulse < 2; pulse++) {
        leds_clear();
        leds_set_mirrored(pulse, c);
        ws_show_armed_frame();
        delay_ms(ARMED_ENTRY_PULSE_MS);

        leds_clear();
        ws_show_armed_frame();
        delay_ms(ARMED_ENTRY_PULSE_MS);
    }
}

static void armed_low_power_mode(uint8_t mode)
{
    uint32_t period_ms = 0;
    uint8_t indicator_on = 0xFF;

    leds_clear();
    ws_show_armed_frame();

    while (1) {
        if (battery_monitor_status().emergency)
            battery_critical_sleep_mode();

        if (btn1_pressed_raw() || btn2_pressed_raw()) {
            delay_ms(DEBOUNCE_MS);
            if (btn1_pressed_raw() || btn2_pressed_raw()) {
                while (btn1_pressed_raw() || btn2_pressed_raw())
                    delay_ms(ARMED_POLL_MS);
                battery_race_ready_show();
                show_color_profile_ramp(mode);
                return;
            }
        }

        uint32_t second_pulse_start = ARMED_INDICATOR_ON_MS +
                                      ARMED_HEARTBEAT_GAP_MS;
        uint8_t new_indicator_on =
            (period_ms < ARMED_INDICATOR_ON_MS) ||
            (period_ms >= second_pulse_start &&
             period_ms < second_pulse_start + ARMED_INDICATOR_ON_MS);

        if (new_indicator_on != indicator_on) {
            leds_clear();
            if (new_indicator_on) {
            Color c = color_scaled(color_profiles[mode],
                                   ARMED_INDICATOR_LEVEL);
            leds_set_mirrored(0, c);
            }
            ws_show_armed_frame();
            indicator_on = new_indicator_on;
        }

        sleep_ms_wfi(ARMED_POLL_MS);
        period_ms += ARMED_POLL_MS;
        if (period_ms >= ARMED_INDICATOR_PERIOD_MS)
            period_ms = 0;
    }
}

static void show_hold_fade(uint8_t mode, uint32_t held_ms)
{
    uint32_t remaining = (held_ms < ARMED_FADE_MS) ?
                         (ARMED_FADE_MS - held_ms) : 0;
    uint8_t brightness = (uint8_t)((remaining * NORMAL_BRIGHTNESS) /
                                   ARMED_FADE_MS);
    Color c = color_scaled(color_profiles[mode], brightness);
    leds_set_all(c.r, c.g, c.b);
    ws_show();
}

static void show_locked_button_indicator(uint8_t mode)
{
    Color c = color_scaled(color_profiles[mode], NORMAL_BRIGHTNESS);
    Color black = {0, 0, 0};

    leds_set_all(c.r, c.g, c.b);
    leds_set_mirrored(0, black);
    ws_show();
}

static void show_armed_ready(uint8_t mode, uint8_t on)
{
    leds_clear();
    if (on) {
        Color c = color_scaled(color_profiles[mode], ARMED_READY_LEVEL);
        leds_set_mirrored(0, c);
        leds_set_mirrored(1, c);
    }
    ws_show_armed_frame();
}

static void calibration_saved_blink(uint8_t mode, uint8_t low_power,
                                    uint8_t blink_count)
{
    Color green = {0, 80, 0};

    for (uint8_t i = 0; i < blink_count; i++) {
        leds_clear();
        leds_set_mirrored(0, green);
        ws_show();
        delay_ms(CALIBRATION_BLINK_MS);

        leds_clear();
        ws_show();
        delay_ms(CALIBRATION_BLINK_MS);
    }

    if (low_power)
        show_low_power_profile(mode, 0);
    else
        show_color_profile(mode);
}

static uint8_t process_button(uint8_t button, uint8_t *mode, uint8_t low_power)
{
    if (!button_pressed_event(button))
        return 0;

    uint32_t held_ms = DEBOUNCE_MS;
    uint32_t next_fade_ms = ARMED_FADE_STEP_MS;
    uint8_t calibration_done = 0;
    uint8_t ready_state = 0xFF;
    while (button_pressed_raw(button)) {
        if (!low_power && next_fade_ms <= ARMED_FADE_MS &&
            held_ms >= next_fade_ms) {
            show_hold_fade(*mode, next_fade_ms);
            next_fade_ms += ARMED_FADE_STEP_MS;
        }

        if (!low_power && held_ms >= ARMED_FADE_MS &&
            held_ms < CALIBRATION_HOLD_MS) {
            uint32_t ready_phase = (held_ms - ARMED_FADE_MS) %
                                   ARMED_READY_PERIOD_MS;
            uint8_t new_ready_state = (ready_phase < ARMED_READY_ON_MS);
            if (new_ready_state != ready_state) {
                show_armed_ready(*mode, new_ready_state);
                ready_state = new_ready_state;
            }
        }

        if (held_ms >= CALIBRATION_RESET_HOLD_MS) {
            battery_full_mv = BATTERY_DEFAULT_FULL_MV;
#if RUNTIME_SAVE_ENABLED
            save_settings(*mode, battery_full_mv);
#endif
            cancel_mode_save();
            calibration_saved_blink(*mode, low_power,
                                    CALIBRATION_RESET_BLINK_COUNT);

            while (button_pressed_raw(button))
                delay_ms(CALIBRATION_POLL_MS);
            return 2;
        }

        if (!calibration_done && held_ms >= CALIBRATION_HOLD_MS) {
            uint16_t measured_mv = battery_monitor_status().millivolts;
            if (measured_mv >= BATTERY_FULL_MIN_MV +
                               BATTERY_CALIBRATION_OFFSET_MV &&
                measured_mv <= BATTERY_FULL_MAX_MV) {
                battery_full_mv = measured_mv -
                                  BATTERY_CALIBRATION_OFFSET_MV;
#if RUNTIME_SAVE_ENABLED
                save_settings(*mode, battery_full_mv);
#endif
                cancel_mode_save();
                calibration_saved_blink(*mode, low_power,
                                        CALIBRATION_BLINK_COUNT);
                held_ms += (uint32_t)CALIBRATION_BLINK_COUNT *
                           CALIBRATION_BLINK_MS * 2U;
            }
            calibration_done = 1;
        }

        delay_ms(CALIBRATION_POLL_MS);
        held_ms += CALIBRATION_POLL_MS;
    }

    if (!low_power && !calibration_done && held_ms >= ARMED_FADE_MS) {
        service_mode_save(MODE_SAVE_DELAY_MS);
        armed_entry_animation(*mode);
        battery_monitor_set_low_tracking(0);
        armed_low_power_mode(*mode);
        battery_monitor_set_low_tracking(1);
        return 2;
    }

    if (calibration_done)
        return 2;

    if (button == 1) {
        (*mode)++;
        if (*mode >= 8)
            *mode = 0;
    } else if (*mode == 0) {
        *mode = 7;
    } else {
        (*mode)--;
    }

    schedule_mode_save(*mode);

    if (low_power)
        show_low_power_profile(*mode, 0);
    else
        show_color_profile(*mode);

    return 1;
}

static uint8_t any_button_pressed_raw(void)
{
    return btn1_pressed_raw() || btn2_pressed_raw();
}

static void wait_for_all_buttons_released(void)
{
    while (any_button_pressed_raw())
        delay_ms(MAIN_LOOP_DELAY_MS);
}

static uint8_t idle_unlock_attempt(void)
{
    if (!any_button_pressed_raw())
        return 0;

    delay_ms(DEBOUNCE_MS);
    if (!any_button_pressed_raw())
        return 0;

    for (uint8_t click = 1; click < UNLOCK_CLICKS; click++) {
        uint32_t interval_ms = DEBOUNCE_MS;

        while (any_button_pressed_raw()) {
            delay_ms(MAIN_LOOP_DELAY_MS);
            interval_ms += MAIN_LOOP_DELAY_MS;
            if (interval_ms > UNLOCK_MAX_INTERVAL_MS) {
                wait_for_all_buttons_released();
                return 0;
            }
        }

        while (!any_button_pressed_raw() &&
               interval_ms < UNLOCK_MAX_INTERVAL_MS) {
            delay_ms(MAIN_LOOP_DELAY_MS);
            interval_ms += MAIN_LOOP_DELAY_MS;
        }

        if (!any_button_pressed_raw() ||
            interval_ms < UNLOCK_MIN_INTERVAL_MS) {
            wait_for_all_buttons_released();
            return 0;
        }

        delay_ms(DEBOUNCE_MS);
        if (!any_button_pressed_raw())
            return 0;
    }

    wait_for_all_buttons_released();
    return 1;
}

static Color disco_color_wheel(uint8_t hue)
{
    Color c;

    if (hue < 85U) {
        c.r = (uint8_t)(255U - hue * 3U);
        c.g = (uint8_t)(hue * 3U);
        c.b = 0;
    } else if (hue < 170U) {
        hue = (uint8_t)(hue - 85U);
        c.r = 0;
        c.g = (uint8_t)(255U - hue * 3U);
        c.b = (uint8_t)(hue * 3U);
    } else {
        hue = (uint8_t)(hue - 170U);
        c.r = (uint8_t)(hue * 3U);
        c.g = 0;
        c.b = (uint8_t)(255U - hue * 3U);
    }

    return c;
}

static uint8_t disco_click_event(void)
{
    for (uint8_t button = 1; button <= 2; button++) {
        if (button_pressed_event(button)) {
            while (button_pressed_raw(button))
                delay_ms(MAIN_LOOP_DELAY_MS);
            return 1;
        }
    }
    return 0;
}

static uint32_t disco_random_next(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void disco_set_sides(uint8_t hue)
{
    Color side_a = disco_color_wheel(hue);
    Color side_b = disco_color_wheel((uint8_t)(hue +
                                  DISCO_COUNTER_HUE_OFFSET));

    for (uint8_t i = 0; i < LED_HALF; i++)
        leds[i] = side_a;
    for (uint8_t i = LED_HALF; i < LED_COUNT; i++)
        leds[i] = side_b;
}

static void disco_rainbow_mode(uint8_t *mode)
{
    enum {
        DISCO_MODE_RAINBOW = 0,
        DISCO_MODE_RANDOM,
        DISCO_MODE_WHITE,
        DISCO_MODE_COUNT
    };

    uint8_t disco_mode = DISCO_MODE_RAINBOW;
    uint32_t phase_accumulator = 0;
    uint32_t random_state = 0x6D2B79F5UL ^
                            battery_monitor_status().millivolts;
    uint32_t effect_elapsed_ms = 0;
    uint8_t effect_on = 1;
    uint8_t second_mode_clicks = 0;
    uint32_t click_gap_ms = 0;
    uint32_t released_ms = 0;
    uint8_t second_mode_armed = 0;

    while (1) {
        if (battery_monitor_status().emergency) {
            battery_critical_sleep_mode();
            return;
        }
        if (battery_monitor_status().low_due) {
            battery_low_power_mode(mode);
            return;
        }

        if (disco_mode == DISCO_MODE_RAINBOW) {
            uint8_t phase = (uint8_t)(phase_accumulator /
                                      DISCO_RAINBOW_CYCLE_MS);
            for (uint8_t i = 0; i < LED_HALF; i++) {
                Color c = disco_color_wheel((uint8_t)(phase +
                                  i * DISCO_RAINBOW_HUE_OFFSET));
                leds_set_mirrored(i, c);
            }
            ws_show();
        } else if (effect_elapsed_ms == 0) {
            if (effect_on) {
                if (disco_mode == DISCO_MODE_RANDOM) {
                    uint8_t hue = (uint8_t)disco_random_next(&random_state);
                    disco_set_sides(hue);
                } else {
                    leds_set_all(255, 255, 255);
                }
            } else {
                leds_clear();
            }
            ws_show();
        }

        uint8_t clicked = disco_click_event();

        if (!second_mode_armed) {
            if (clicked) {
                released_ms = 0;
            } else if (!btn1_pressed_raw() && !btn2_pressed_raw()) {
                released_ms += DISCO_RAINBOW_FRAME_MS;
                if (released_ms >= DISCO_SECOND_MODE_ARM_MS) {
                    second_mode_armed = 1;
                    second_mode_clicks = 0;
                    click_gap_ms = 0;
                }
            } else {
                released_ms = 0;
            }
        } else if (clicked) {
            if (click_gap_ms == 0)
                second_mode_clicks = 0;
            second_mode_clicks++;
            click_gap_ms = DISCO_CLICK_GAP_MS;
            if (second_mode_clicks >= DISCO_SECOND_MODE_CLICKS) {
                disco_mode = (uint8_t)((disco_mode + 1U) % DISCO_MODE_COUNT);
                second_mode_armed = 0;
                second_mode_clicks = 0;
                click_gap_ms = 0;
                released_ms = 0;
                effect_elapsed_ms = 0;
                effect_on = 1;
                phase_accumulator = 0;

                if (disco_mode == DISCO_MODE_RANDOM) {
                    uint8_t hue = (uint8_t)disco_random_next(&random_state);
                    disco_set_sides(hue);
                    ws_show();
                } else if (disco_mode == DISCO_MODE_WHITE) {
                    leds_set_all(255, 255, 255);
                    ws_show();
                }
            }
        }

        delay_ms(DISCO_RAINBOW_FRAME_MS);

        if (disco_mode == DISCO_MODE_RAINBOW) {
            phase_accumulator += 256UL * DISCO_RAINBOW_FRAME_MS;
            phase_accumulator %= (256UL * DISCO_RAINBOW_CYCLE_MS);
        } else {
            uint32_t interval_ms;
            if (disco_mode == DISCO_MODE_RANDOM)
                interval_ms = effect_on ? DISCO_RANDOM_ON_MS :
                                          DISCO_RANDOM_OFF_MS;
            else
                interval_ms = effect_on ? DISCO_WHITE_ON_MS :
                                          DISCO_WHITE_OFF_MS;

            effect_elapsed_ms += DISCO_RAINBOW_FRAME_MS;
            if (effect_elapsed_ms >= interval_ms) {
                effect_elapsed_ms = 0;
                effect_on = !effect_on;
            }
        }

        if (click_gap_ms > DISCO_RAINBOW_FRAME_MS)
            click_gap_ms -= DISCO_RAINBOW_FRAME_MS;
        else
            click_gap_ms = 0;
    }
}

static uint8_t battery_checker_requested(uint8_t held_buttons)
{
    if (held_buttons == 0)
        return 0;

    uint32_t held_ms = 0;
    while (held_ms < CHECKER_ENTRY_HOLD_MS) {
        if (!btn1_pressed_raw())
            held_buttons &= (uint8_t)~1U;
        if (!btn2_pressed_raw())
            held_buttons &= (uint8_t)~2U;
        if (held_buttons == 0)
            return 0;

        uint16_t mv = battery_monitor_status().millivolts;
        uint8_t target_brightness = checker_bar_brightness(mv);
        uint32_t remaining_ms = CHECKER_ENTRY_HOLD_MS - held_ms;
        uint8_t brightness = (uint8_t)(target_brightness +
            ((uint32_t)(BATTERY_DISPLAY_BRIGHTNESS - target_brightness) *
             remaining_ms) /
            CHECKER_ENTRY_HOLD_MS);

        checker_bargraph_show(mv, brightness);
        if (battery_protection_wait(CHECKER_ENTRY_FRAME_MS, 1, 0) == 1)
            battery_critical_sleep_mode();
        held_ms += CHECKER_ENTRY_FRAME_MS;
    }

    /* Do not enter if the button was released during the final fade frame. */
    if ((!btn1_pressed_raw() && (held_buttons & 1U)) ||
        (!btn2_pressed_raw() && (held_buttons & 2U)))
        return 0;

    /* A short off/on blink separates the entry fade from persistent mode. */
    leds_clear();
    ws_show();
    battery_protection_wait(CHECKER_ENTRY_BLINK_MS, 1, 0);

    uint16_t mv = battery_monitor_status().millivolts;
    checker_bargraph_show(mv, checker_bar_brightness(mv));
    battery_protection_wait(CHECKER_ENTRY_BLINK_MS, 1, 0);

    return 1;
}

static void battery_checker_mode(void)
{
    uint32_t warning_elapsed_ms = 0;
    uint32_t button_hold_ms = 0;
    uint8_t brightness_control_armed = 0;

    while (1) {
        if (battery_monitor_status().emergency)
            battery_critical_sleep_mode();

        uint8_t button_pressed = any_button_pressed_raw();
        if (!brightness_control_armed && !button_pressed)
            brightness_control_armed = 1;

        uint16_t mv = battery_monitor_status().millivolts;

        if (mv > CHECKER_WARNING_MV) {
            warning_elapsed_ms = 0;

            uint8_t brightness = checker_bar_brightness(mv);
            if (brightness_control_armed && button_pressed) {
                if (button_hold_ms < CHECKER_HOLD_RAMP_MS) {
                    button_hold_ms += CHECKER_UPDATE_MS;
                    if (button_hold_ms > CHECKER_HOLD_RAMP_MS)
                        button_hold_ms = CHECKER_HOLD_RAMP_MS;
                }

                brightness = (uint8_t)(brightness +
                    (button_hold_ms * (255U - brightness)) /
                    CHECKER_HOLD_RAMP_MS);
            } else {
                button_hold_ms = 0;
            }

            checker_bargraph_show(mv, brightness);
            battery_protection_wait(CHECKER_UPDATE_MS, 1, 0);
            continue;
        }

        button_hold_ms = 0;

        uint32_t brightness = CHECKER_BRIGHTNESS +
            (warning_elapsed_ms * (255U - CHECKER_BRIGHTNESS)) /
            CHECKER_WARNING_RAMP_MS;
        if (brightness > 255U)
            brightness = 255U;

        for (uint8_t blink = 0; blink < 2; blink++) {
            leds_set_all((uint8_t)brightness, 0, 0);
            ws_show();
            battery_protection_wait(CHECKER_BLINK_ON_MS, 1, 0);

            leds_clear();
            ws_show();
            if (blink == 0)
                battery_protection_wait(CHECKER_BLINK_GAP_MS, 1, 0);
        }

        battery_protection_wait(CHECKER_BLINK_OFF_MS, 1, 0);

        if (warning_elapsed_ms < CHECKER_WARNING_RAMP_MS) {
            warning_elapsed_ms += CHECKER_BLINK_ON_MS * 2U +
                                  CHECKER_BLINK_GAP_MS +
                                  CHECKER_BLINK_OFF_MS;
            if (warning_elapsed_ms > CHECKER_WARNING_RAMP_MS)
                warning_elapsed_ms = CHECKER_WARNING_RAMP_MS;
        }
    }
}

// =========================
// Battery low / critical modes
// =========================

static void battery_low_power_mode(uint8_t *mode)
{
    battery_monitor_set_low_tracking(0);

    while (1) {
        if (battery_monitor_status().emergency) {
            battery_critical_sleep_mode();
            return;
        }

        if (battery_monitor_status().recovered)
            return;

        process_button(1, mode, 1);
        process_button(2, mode, 1);

        show_low_power_profile(*mode, 1);
        if (battery_protection_wait(LOW_POWER_BLINK_ON_MS, 1, 1) != 0)
            continue;

        show_low_power_profile(*mode, 0);
        if (battery_protection_wait(LOW_POWER_BLINK_OFF_MS, 1, 1) != 0)
            continue;
        service_mode_save(LOW_POWER_BLINK_ON_MS + LOW_POWER_BLINK_OFF_MS);
    }
}

static void battery_critical_sleep_mode(void)
{
    battery_monitor_set_low_tracking(0);

    while (1) {
        if (battery_monitor_status().recovered) {
            battery_monitor_resume_after_recovery();
            return;
        }

        leds_clear();
        ws_show();

        if (battery_protection_wait(CRITICAL_SLEEP_MS, 0, 1) == 2)
            continue;

        leds_clear();

        Color red = {CRITICAL_BLINK_LEVEL, 0, 0};
        leds_set_mirrored(0, red);
        ws_show();

        battery_protection_wait(CRITICAL_BLINK_ON_MS, 0, 1);

        leds_clear();
        ws_show();
    }
}

// =========================
// Main
// =========================

int main(void)
{
    gpio_init();
    led_output_init();
    battery_monitor_init();

    uint8_t mode = load_saved_settings(&battery_full_mv);
    uint8_t startup_buttons = 0;
    if (btn1_pressed_raw())
        startup_buttons |= 1U;
    if (btn2_pressed_raw())
        startup_buttons |= 2U;

    boot_sweep_white();

    if (battery_checker_requested(startup_buttons)) {
        battery_monitor_set_low_tracking(0);
        battery_checker_mode();
    }

    /* A held startup button owns the battery-bar presentation while checker
     * entry is being decided. Only run the normal ready indication after the
     * button is released before the checker hold threshold. */
    battery_race_ready_show();

    if (battery_monitor_status().emergency)
        battery_critical_sleep_mode();

    battery_monitor_set_low_tracking(1);
    show_color_profile_ramp(mode);

    uint8_t rapid_clicks = 0;
    uint32_t rapid_click_gap_ms = 0;
    uint32_t idle_ms = 0;
    uint8_t input_locked = 0;

    while (1)
    {
        if (battery_monitor_status().emergency) {
            battery_critical_sleep_mode();
            battery_monitor_set_low_tracking(1);
            show_color_profile(mode);
        } else if (battery_monitor_status().low_due) {
            battery_low_power_mode(&mode);
            battery_monitor_set_low_tracking(1);
            show_color_profile(mode);
        }

        if (input_locked) {
            if (any_button_pressed_raw()) {
                show_locked_button_indicator(mode);

                if (idle_unlock_attempt()) {
                    input_locked = 0;
                    idle_ms = 0;
                    rapid_clicks = 0;
                    rapid_click_gap_ms = 0;
                    calibration_saved_blink(mode, 0, 1);
                } else {
                    show_color_profile(mode);
                }
            }

            delay_ms(MAIN_LOOP_DELAY_MS);
            service_mode_save(MAIN_LOOP_DELAY_MS);
            continue;
        }

        uint8_t action1 = process_button(1, &mode, 0);
        uint8_t action2 = process_button(2, &mode, 0);
        uint8_t clicked = (action1 == 1U) || (action2 == 1U);
        uint8_t button_activity = (action1 != 0U) || (action2 != 0U);

        if (button_activity) {
            idle_ms = 0;
        } else if (idle_ms < IDLE_LOCK_MS) {
            idle_ms += MAIN_LOOP_DELAY_MS;
            if (idle_ms >= IDLE_LOCK_MS) {
                input_locked = 1;
                rapid_clicks = 0;
                rapid_click_gap_ms = 0;
            }
        }

        if (clicked) {
            if (rapid_click_gap_ms == 0)
                rapid_clicks = 0;
            rapid_clicks++;
            rapid_click_gap_ms = DISCO_CLICK_GAP_MS;

            if (rapid_clicks >= DISCO_TRIGGER_CLICKS) {
                cancel_mode_save();
                disco_rainbow_mode(&mode);
                battery_monitor_set_low_tracking(1);
                show_color_profile(mode);
            }
        } else if (rapid_click_gap_ms > MAIN_LOOP_DELAY_MS) {
            rapid_click_gap_ms -= MAIN_LOOP_DELAY_MS;
        } else {
            rapid_click_gap_ms = 0;
        }

        delay_ms(MAIN_LOOP_DELAY_MS);
        service_mode_save(MAIN_LOOP_DELAY_MS);
    }
}
