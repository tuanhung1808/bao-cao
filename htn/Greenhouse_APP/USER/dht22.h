#ifndef DHT22_H
#define DHT22_H
#include <stdint.h>
uint8_t DHT22_Read(int16_t *temperatureC, uint8_t *humidityPercent);
#endif
