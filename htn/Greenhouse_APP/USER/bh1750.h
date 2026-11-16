#ifndef BH1750_H
#define BH1750_H
#include <stdint.h>
uint8_t BH1750_Init(void);
uint8_t BH1750_ReadLux(uint16_t *lux);
#endif
