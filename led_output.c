#include "led_output.h"

#include "platform_irq.h"
#include "py32f0xx.h"

#define SYSTEM_CLOCK_HZ         24000000UL
#define SYSTICK_TICKS_PER_US    (SYSTEM_CLOCK_HZ / 1000000UL)
#define PERIPHERAL_TIMEOUT_LOOPS 100000UL

#define LED_DATA_PIN            7U
#define LED_OUTPUT_COUNT        64U

#define WS_ZERO                 0x8U
#define WS_ONE                  0xCU

/* Set to zero only when bisecting LED-transmission-related hardware faults. */
#define LED_OUTPUT_ENABLED      1

static const uint8_t ws_encode2[4] = {
    (WS_ZERO << 4) | WS_ZERO,
    (WS_ZERO << 4) | WS_ONE,
    (WS_ONE  << 4) | WS_ZERO,
    (WS_ONE  << 4) | WS_ONE,
};

static void latch_delay_us(uint32_t us)
{
    SysTick->LOAD = (SYSTICK_TICKS_PER_US * us) - 1U;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0) {
    }

    SysTick->CTRL = 0;
}

static uint8_t spi_send_byte(uint8_t data)
{
    uint32_t timeout = PERIPHERAL_TIMEOUT_LOOPS;
    while ((SPI1->SR & SPI_SR_TXE) == 0) {
        if (--timeout == 0)
            return 0;
    }

    *((volatile uint8_t *)&SPI1->DR) = data;

    timeout = PERIPHERAL_TIMEOUT_LOOPS;
    while ((SPI1->SR & SPI_SR_BSY) != 0) {
        if (--timeout == 0)
            return 0;
    }
    return 1;
}

static uint8_t ws_send_byte(uint8_t data)
{
    return spi_send_byte(ws_encode2[(data >> 6) & 0x03U]) &&
           spi_send_byte(ws_encode2[(data >> 4) & 0x03U]) &&
           spi_send_byte(ws_encode2[(data >> 2) & 0x03U]) &&
           spi_send_byte(ws_encode2[data & 0x03U]);
}

static uint8_t ws_send_color(Color color)
{
    return ws_send_byte(color.g) &&
           ws_send_byte(color.r) &&
           ws_send_byte(color.b);
}

void led_output_init(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    GPIOA->MODER &= ~(3U << (LED_DATA_PIN * 2U));
    GPIOA->MODER |=  (2U << (LED_DATA_PIN * 2U));
    GPIOA->OTYPER &= ~(1U << LED_DATA_PIN);
    GPIOA->OSPEEDR &= ~(3U << (LED_DATA_PIN * 2U));
    GPIOA->OSPEEDR |=  (3U << (LED_DATA_PIN * 2U));
    GPIOA->PUPDR &= ~(3U << (LED_DATA_PIN * 2U));
    GPIOA->AFR[0] &= ~(0xFU << (LED_DATA_PIN * 4U));

    RCC->APBENR2 |= RCC_APBENR2_SPI1EN;

    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 = SPI_CR1_MSTR |
                SPI_CR1_SSM  |
                SPI_CR1_SSI  |
                SPI_CR1_BR_1;
    SPI1->CR2 = 0;

#ifdef SPI_CR2_DS
    SPI1->CR2 |= (7U << SPI_CR2_DS_Pos);
#endif
#ifdef SPI_CR2_FRXTH
    SPI1->CR2 |= SPI_CR2_FRXTH;
#endif

    SPI1->CR1 |= SPI_CR1_SPE;
}

void led_output_show(const LedFrame frame, LedOutputMode mode)
{
#if LED_OUTPUT_ENABLED
    uint32_t primask = platform_irq_save_and_disable();

    for (uint8_t i = 0; i < LED_OUTPUT_COUNT; i++) {
        Color color = {0, 0, 0};

        if (mode == LED_OUTPUT_REPEAT_FRAME)
            color = frame[i % LED_COUNT];
        else if (i < LED_COUNT)
            color = frame[i];

        if (!ws_send_color(color))
            break;
    }

    platform_irq_restore(primask);
    latch_delay_us(100U);
#else
    (void)frame;
    (void)mode;
#endif
}
