#ifndef __IP_TIMER_H
#include "stm32f10x.h"

void TIMER_Configure(uint16_t pin, TIM_TypeDef *TIMx);
void delay_ms(int time, TIM_TypeDef *TIMx);
void delay_s(int time, TIM_TypeDef *TIMx);
void delay_us(int time, TIM_TypeDef *TIMx);
void TIMER_PWM(uint32_t SpeedDongco1, uint32_t SpeedDongco2, uint32_t SpeedDongco3, uint32_t SpeedDongco4);
void TIMER_PWM_Servo(GPIO_TypeDef *GPIOx, int pin, int speed);
void TIMER_PWM_Led(GPIO_TypeDef *GPIOx, int pin, int speed);

#endif

