#include "battery_monitor.h"

#include "py32f0xx.h"

#define SYSTEM_CLOCK_HZ              24000000UL
#define SYSTICK_TICKS_PER_US          (SYSTEM_CLOCK_HZ / 1000000UL)
#define PERIPHERAL_TIMEOUT_LOOPS      100000UL

#define ADC_PIN                       1U
#define VREFINT_MV                    1225UL
#define ADC_AVERAGE_SAMPLES           12U
#define ADC_STARTUP_DELAY_US          20U
#define ADC_ENABLE_DELAY_US           10U
#define ADC_MONITOR_SETTLE_US         20000U

#define BATTERY_FILTER_WINDOW_MS      2000U
#define BATTERY_FILTER_SAMPLES        (BATTERY_FILTER_WINDOW_MS / \
                                       BATTERY_SAMPLE_INTERVAL_MS)
#define BATTERY_RECOVERY_MV           3500U
#define BATTERY_LOW_HOLD_MS           10000U
#define BATTERY_RECOVERY_HOLD_MS      500U
#define BATTERY_EMERGENCY_MV          2800U
#define BATTERY_EMERGENCY_HOLD_MS     200U

typedef struct {
    uint16_t filter_values[BATTERY_FILTER_SAMPLES];
    uint32_t filter_sum;
    uint16_t filtered_mv;
    uint16_t last_valid_mv;
    uint8_t filter_index;
    uint8_t initialized;
    uint8_t low_tracking_enabled;
    uint32_t sample_elapsed_ms;
    uint32_t low_elapsed_ms;
    uint32_t recovery_elapsed_ms;
    uint32_t emergency_elapsed_ms;
    uint8_t emergency_active;
} BatteryMonitor;

static BatteryMonitor monitor;

static void delay_us(uint32_t us)
{
    SysTick->LOAD = (SYSTICK_TICKS_PER_US * us) - 1U;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0) {
    }

    SysTick->CTRL = 0;
}

static void adc_init_vrefint(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
    GPIOB->MODER &= ~(3U << (ADC_PIN * 2U));
    GPIOB->MODER |=  (3U << (ADC_PIN * 2U));
    GPIOB->PUPDR &= ~(3U << (ADC_PIN * 2U));

    RCC->APBENR2 |= (1U << 20);
    ADC->CCR |= (1U << 22);
    delay_us(ADC_STARTUP_DELAY_US);

    ADC1->CHSELR = (1U << 9);
    ADC1->SMPR = 0x7;
    ADC1->CR |= ADC_CR_ADEN;
    delay_us(ADC_ENABLE_DELAY_US);
}

static uint16_t adc_read_vrefint_raw(void)
{
    uint32_t timeout = PERIPHERAL_TIMEOUT_LOOPS;

    ADC1->CHSELR = (1U << 9);
    ADC1->ISR = 0xFFFFFFFF;
    ADC1->CR |= ADC_CR_ADSTART;

    while ((ADC1->ISR & ADC_ISR_EOC) == 0) {
        if (--timeout == 0)
            return 0;
    }

    return (uint16_t)(ADC1->DR & 0x0FFF);
}

static uint16_t battery_mv_read(void)
{
    uint32_t sum = 0;
    uint8_t valid_samples = 0;

    for (uint8_t i = 0; i < ADC_AVERAGE_SAMPLES; i++) {
        uint16_t sample = adc_read_vrefint_raw();
        if (sample != 0) {
            sum += sample;
            valid_samples++;
        }
    }

    if (valid_samples == 0)
        return monitor.last_valid_mv;

    uint32_t raw = sum / valid_samples;
    if (raw == 0)
        return monitor.last_valid_mv;

    monitor.last_valid_mv = (uint16_t)((VREFINT_MV * 4095UL) / raw);
    return monitor.last_valid_mv;
}

static void battery_monitor_sample(void)
{
    uint16_t raw_mv = battery_mv_read();

    if (!monitor.initialized) {
        monitor.filter_sum = (uint32_t)raw_mv * BATTERY_FILTER_SAMPLES;
        for (uint16_t i = 0; i < BATTERY_FILTER_SAMPLES; i++)
            monitor.filter_values[i] = raw_mv;
        monitor.filtered_mv = raw_mv;
        monitor.initialized = 1;
    } else {
        monitor.filter_sum -= monitor.filter_values[monitor.filter_index];
        monitor.filter_values[monitor.filter_index] = raw_mv;
        monitor.filter_sum += raw_mv;
        monitor.filter_index++;
        if (monitor.filter_index >= BATTERY_FILTER_SAMPLES)
            monitor.filter_index = 0;
        monitor.filtered_mv = (uint16_t)(monitor.filter_sum /
                                         BATTERY_FILTER_SAMPLES);
    }

    if (raw_mv < BATTERY_EMERGENCY_MV) {
        if (monitor.emergency_elapsed_ms < BATTERY_EMERGENCY_HOLD_MS)
            monitor.emergency_elapsed_ms += BATTERY_SAMPLE_INTERVAL_MS;
        if (monitor.emergency_elapsed_ms >= BATTERY_EMERGENCY_HOLD_MS &&
            !monitor.emergency_active) {
            monitor.emergency_active = 1;
            monitor.recovery_elapsed_ms = 0;
        }
    } else {
        monitor.emergency_elapsed_ms = 0;
    }

    if (monitor.filtered_mv > BATTERY_RECOVERY_MV) {
        if (monitor.recovery_elapsed_ms < BATTERY_RECOVERY_HOLD_MS)
            monitor.recovery_elapsed_ms += BATTERY_SAMPLE_INTERVAL_MS;
        if (monitor.recovery_elapsed_ms >= BATTERY_RECOVERY_HOLD_MS)
            monitor.low_elapsed_ms = 0;
    } else {
        monitor.recovery_elapsed_ms = 0;
    }

    if (monitor.low_tracking_enabled &&
        monitor.filtered_mv < BATTERY_LOW_MV &&
        monitor.low_elapsed_ms < BATTERY_LOW_HOLD_MS) {
        monitor.low_elapsed_ms += BATTERY_SAMPLE_INTERVAL_MS;
    }
}

void battery_monitor_init(void)
{
    monitor.filtered_mv = BATTERY_DEFAULT_FULL_MV;
    monitor.last_valid_mv = BATTERY_DEFAULT_FULL_MV;
    adc_init_vrefint();
    delay_us(ADC_MONITOR_SETTLE_US);
    battery_monitor_sample();
    monitor.sample_elapsed_ms = 0;
}

void battery_monitor_tick(uint32_t elapsed_ms)
{
    if (!monitor.initialized)
        return;

    monitor.sample_elapsed_ms += elapsed_ms;
    while (monitor.sample_elapsed_ms >= BATTERY_SAMPLE_INTERVAL_MS) {
        monitor.sample_elapsed_ms -= BATTERY_SAMPLE_INTERVAL_MS;
        battery_monitor_sample();
    }
}

void battery_monitor_set_low_tracking(uint8_t enabled)
{
    monitor.low_tracking_enabled = enabled;
    if (!enabled)
        monitor.low_elapsed_ms = 0;
}

BatteryStatus battery_monitor_status(void)
{
    BatteryStatus status = {
        monitor.filtered_mv,
        monitor.low_elapsed_ms >= BATTERY_LOW_HOLD_MS,
        monitor.recovery_elapsed_ms >= BATTERY_RECOVERY_HOLD_MS,
        monitor.emergency_active
    };
    return status;
}

void battery_monitor_resume_after_recovery(void)
{
    if (monitor.recovery_elapsed_ms < BATTERY_RECOVERY_HOLD_MS)
        return;

    monitor.emergency_active = 0;
    monitor.emergency_elapsed_ms = 0;
}
