#include "SYSTICK.h"
volatile uint32_t systick_ms = 0;
void SysTick_Init(void)
{
    SystemCoreClockUpdate();
    SysTick->LOAD = SystemCoreClock / 1000u - 1u;
    SysTick->VAL = 0;
    SysTick->CTRL = (1u << 2) | (1u << 1) | 1u;
}
void SysTick_Handler(void) { ++systick_ms; }
uint32_t millis(void) { return systick_ms; }
void delay_ms(uint32_t ms)
{
    uint32_t start = millis();
    while ((uint32_t)(millis() - start) < ms) { }
}
