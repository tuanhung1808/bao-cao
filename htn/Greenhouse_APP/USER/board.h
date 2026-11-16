#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include "stm32f10x.h"

void Board_Init(void);
void Board_ADC_Init(void);
void Board_I2C1_Init(void);
void Board_UART1_Init(uint32_t baud);
void Board_UART1_EnableRxInterrupt(void);
void Board_TIM2_Init1MHz(void);
void Board_DelayUs(uint16_t us);
void Board_DelayMs(uint32_t ms);
uint8_t Board_ButtonPressed(uint8_t index);
void Board_SetPump(uint8_t on);
void Board_SetFan(uint8_t on);
void Board_SetLamp(uint8_t on);
void Board_SetRelay4(uint8_t on);
void Board_SetBuzzer(uint8_t on);
void Board_SetStatusLED(uint8_t on);
void Board_AllRelaysOff(void);
void Board_UART1_SendString(const char *s);
uint8_t Board_ADC_Read(uint8_t index, uint16_t *value);

#endif
