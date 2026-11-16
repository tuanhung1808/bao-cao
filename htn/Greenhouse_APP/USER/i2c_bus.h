#ifndef I2C_BUS_H
#define I2C_BUS_H
#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
void I2C1_SetMutex(SemaphoreHandle_t mutex);
uint8_t I2C1_WriteBytes(uint8_t address7, const uint8_t *data, uint16_t length);
uint8_t I2C1_ReadRegisters(uint8_t address7, uint8_t reg, uint8_t *data, uint16_t length);
uint8_t I2C1_ReadBytes(uint8_t address7, uint8_t *data, uint16_t length);
#endif
