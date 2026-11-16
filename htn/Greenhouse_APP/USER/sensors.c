#include "sensors.h"
#include "app_config.h"
#include "board.h"
#include "dht22.h"
#include "bh1750.h"
#include "ds3231.h"

static uint8_t s_lightReady = 0U;
static uint8_t s_dhtFailures = 0U;

static uint8_t raw_to_soil_percent(uint16_t raw)
{
    int32_t numerator;
    int32_t denominator;
    int32_t percent;
    denominator = (int32_t)SOIL_RAW_DRY - (int32_t)SOIL_RAW_WET;
    if (denominator == 0) return 0U;
    numerator = ((int32_t)SOIL_RAW_DRY - (int32_t)raw) * 100;
    percent = numerator / denominator;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return (uint8_t)percent;
}

void Sensors_Init(void)
{
    s_lightReady = BH1750_Init();
}

void Sensors_ReadFast(SensorData_t *data)
{
    uint16_t value;
    if (data == 0) return;
    if (Board_ADC_Read(0U, &value))
    {
        data->soilRaw = value;
        data->soilPercent = raw_to_soil_percent(value);
    }
    if (Board_ADC_Read(1U, &value))
    {
        data->waterRaw = value;
        /* The common resistive water strip produces a larger ADC value when wet.
         * Calibrate WATER_RAW_MIN after testing the actual module and tank level.
         */
        data->waterOK = (value >= WATER_RAW_MIN) ? 1U : 0U;
    }
}

void Sensors_ReadDHT22(SensorData_t *data)
{
    int16_t temperature;
    uint8_t humidity;
    if (data == 0) return;
    if (DHT22_Read(&temperature, &humidity))
    {
        data->temperatureC = temperature;
        data->humidityPercent = humidity;
        data->temperatureValid = 1U;
        data->dhtValid = 1U;
        s_dhtFailures = 0U;
    }
    else
    {
        data->dhtValid = 0U;
        if (s_dhtFailures < 3U) s_dhtFailures++;
        /* Keep the last value for display, but after three missed reads mark it
         * invalid so the pump cannot start without temperature safety data. */
        if (s_dhtFailures >= 3U) data->temperatureValid = 0U;
    }
}

void Sensors_ReadLight(SensorData_t *data)
{
    uint16_t lux;
    if (data == 0) return;
    if (s_lightReady && BH1750_ReadLux(&lux))
    {
        data->lightLux = lux;
        data->lightValid = 1U;
    }
    else
    {
        data->lightValid = 0U;
    }
}

void Sensors_ReadRTC(SensorData_t *data)
{
    DS3231_Time_t t;
    if (data == 0) return;
    if (DS3231_ReadTime(&t))
    {
        data->second = t.second;
        data->minute = t.minute;
        data->hour = t.hour;
        data->day = t.day;
        data->month = t.month;
        data->year = t.year;
        data->dayKey = ((uint32_t)t.year * 10000UL) + ((uint32_t)t.month * 100UL) + t.day;
        data->rtcValid = 1U;
    }
    else
    {
        data->rtcValid = 0U;
    }
}
