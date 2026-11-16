#ifndef OLED_H
#define OLED_H
#include <stdint.h>
void OLED_Init(void);
void OLED_Clear(void);
void OLED_WriteLine(uint8_t page, const char *text);
#endif
