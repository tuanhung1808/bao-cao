#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#define APP_CONFIG_MAGIC  0x47484346UL /* 'GHCF' */
#define APP_CONFIG_VERSION 1UL

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t soilOnPercent;
    uint32_t soilOffPercent;
    uint32_t fanOnTempC;
    uint32_t waterBlockTempC;
    uint32_t lightOnLux;
    uint32_t schedule1Minute;  /* minute of day: 0..1439 */
    uint32_t schedule2Minute;
    uint32_t crc32;
} AppConfig_t;

typedef struct
{
    uint16_t soilRaw;
    uint8_t soilPercent;
    uint16_t waterRaw;
    uint8_t waterOK;
    int16_t temperatureC;
    uint8_t temperatureValid;
    uint8_t humidityPercent;
    uint8_t dhtValid;
    uint16_t lightLux;
    uint8_t lightValid;
    uint8_t rtcValid;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint32_t dayKey;
} SensorData_t;

typedef enum
{
    BUTTON_OK = 0,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_BACK
} ButtonEvent_t;

extern QueueHandle_t g_buttonQueue;
extern QueueHandle_t g_sensorQueue;
extern QueueHandle_t g_configSaveQueue;
extern QueueHandle_t g_uartRxQueue;
extern SemaphoreHandle_t g_configMutex;
extern SemaphoreHandle_t g_i2cMutex;
extern volatile uint8_t g_configSaveState; /* 0 idle, 1 saving, 2 success, 3 error */
extern volatile uint8_t g_pumpOn;
extern volatile uint8_t g_fanOn;
extern volatile uint8_t g_lampOn;
extern volatile uint8_t g_tankLowAlarm;
extern volatile uint8_t g_manualMode;
extern volatile uint8_t g_manualPumpRequest;
extern volatile uint8_t g_manualFanRequest;
extern volatile uint8_t g_manualLampRequest;

void AppConfig_Default(AppConfig_t *config);
uint8_t AppConfig_Validate(const AppConfig_t *config);
void AppConfig_GetActive(AppConfig_t *config);
void AppConfig_SetActive(const AppConfig_t *config);

#endif
