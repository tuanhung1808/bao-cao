#ifndef __LCD_H
#define __LCD_H

#include "stm32f10x.h"
#include "I2C.h"
#include "GPIO.h"

void LCD_Init(void);
void LCD_Clear(void);
void LCD_GotoXY(uint8_t row, uint8_t col);
void LCD_Print(char *str);

void keypad_seclect_row(uint8_t row);

void LCD_SendData(uint8_t data);


void LCD_Print_Number(uint16_t num);


#endif

