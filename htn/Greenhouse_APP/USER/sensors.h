#ifndef SENSORS_H
#define SENSORS_H
#include <stdint.h>
#include "app_types.h"
void Sensors_Init(void);
void Sensors_ReadFast(SensorData_t *data);
void Sensors_ReadDHT22(SensorData_t *data);
void Sensors_ReadLight(SensorData_t *data);
void Sensors_ReadRTC(SensorData_t *data);
#endif
