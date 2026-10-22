#include "PWM.h"
#include "motor_config.h"
void PWM_config(void)
{
    uint32_t code = (RCC->CFGR >> 8) & 7u;
    uint32_t div = code < 4u ? 1u : (1u << (code - 3u));
    uint32_t clock = SystemCoreClock / div;
    if (div != 1u) clock *= 2u;
    TIM3->CR1 = 0;
    TIM3->PSC = 0;
    TIM3->ARR = clock / PWM_FREQUENCY_HZ - 1u;
    TIM3->CCR1 = 0;
    TIM3->CCMR1 = (6u << 4) | (1u << 3);
    TIM3->CCER = 1u;
    TIM3->CR1 = 1u << 7;
    TIM3->EGR = 1u;
    TIM3->CR1 |= 1u;
}
