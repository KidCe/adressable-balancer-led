#ifndef PLATFORM_IRQ_H
#define PLATFORM_IRQ_H

#include "py32f0xx.h"

static inline uint32_t platform_irq_save_and_disable(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void platform_irq_restore(uint32_t primask)
{
    if ((primask & 1U) == 0U)
        __enable_irq();
}

#endif
