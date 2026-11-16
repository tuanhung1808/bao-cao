#include "app_types.h"
#include "app_config.h"
#include "task.h"
#include <string.h>

QueueHandle_t g_buttonQueue = NULL;
QueueHandle_t g_sensorQueue = NULL;
QueueHandle_t g_configSaveQueue = NULL;
QueueHandle_t g_uartRxQueue = NULL;
SemaphoreHandle_t g_configMutex = NULL;
SemaphoreHandle_t g_i2cMutex = NULL;
volatile uint8_t g_configSaveState = 0U;
volatile uint8_t g_pumpOn = 0U;
volatile uint8_t g_fanOn = 0U;
volatile uint8_t g_lampOn = 0U;
volatile uint8_t g_tankLowAlarm = 0U;
volatile uint8_t g_manualMode = 0U; /* 0=AUTO, 1=MANUAL; boots safe in AUTO */
volatile uint8_t g_manualPumpRequest = 0U;
volatile uint8_t g_manualFanRequest = 0U;
volatile uint8_t g_manualLampRequest = 0U;

static AppConfig_t s_activeConfig;

void AppConfig_Default(AppConfig_t *c)
{
    if (c == 0) return;
    memset(c, 0, sizeof(*c));
    c->magic = APP_CONFIG_MAGIC;
    c->version = APP_CONFIG_VERSION;
    c->soilOnPercent = DEFAULT_SOIL_ON_PERCENT;
    c->soilOffPercent = DEFAULT_SOIL_OFF_PERCENT;
    c->fanOnTempC = DEFAULT_FAN_TEMP_C;
    c->waterBlockTempC = DEFAULT_WATER_BLOCK_TEMP_C;
    c->lightOnLux = DEFAULT_LIGHT_ON_LUX;
    c->schedule1Minute = DEFAULT_SCHEDULE_1_MIN;
    c->schedule2Minute = DEFAULT_SCHEDULE_2_MIN;
}

uint8_t AppConfig_Validate(const AppConfig_t *c)
{
    if (c == 0) return 0U;
    if (c->magic != APP_CONFIG_MAGIC || c->version != APP_CONFIG_VERSION) return 0U;
    if (c->soilOnPercent < 5U || c->soilOnPercent > 70U) return 0U;
    if (c->soilOffPercent <= c->soilOnPercent || c->soilOffPercent > 95U) return 0U;
    if (c->fanOnTempC < 15U || c->fanOnTempC > 50U) return 0U;
    if (c->waterBlockTempC < 20U || c->waterBlockTempC > 60U) return 0U;
    if (c->lightOnLux > 2000U) return 0U;
    if (c->schedule1Minute >= 1440U || c->schedule2Minute >= 1440U) return 0U;
    return 1U;
}

void AppConfig_GetActive(AppConfig_t *c)
{
    if (c == 0) return;
    if ((g_configMutex != NULL) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
        (void)xSemaphoreTake(g_configMutex, portMAX_DELAY);
    *c = s_activeConfig;
    if ((g_configMutex != NULL) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
        (void)xSemaphoreGive(g_configMutex);
}

void AppConfig_SetActive(const AppConfig_t *c)
{
    if (c == 0) return;
    if ((g_configMutex != NULL) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
        (void)xSemaphoreTake(g_configMutex, portMAX_DELAY);
    s_activeConfig = *c;
    if ((g_configMutex != NULL) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
        (void)xSemaphoreGive(g_configMutex);
}
