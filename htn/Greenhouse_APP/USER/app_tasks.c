#include "app_tasks.h"
#include "app_types.h"
#include "app_config.h"
#include "app_state.h"
#include "board.h"
#include "i2c_bus.h"
#include "sensors.h"
#include "oled.h"
#include "config_store.h"
#include "ds3231.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "misc.h"
#include "stm32f10x_usart.h"
#include <string.h>
#include <stdio.h>

/* ---------------------------- Task prototypes ---------------------------- */
static void SensorTask(void *argument);
static void ControlTask(void *argument);
static void ButtonTask(void *argument);
static void UITask(void *argument);
static void ConfigTask(void *argument);
static void CommTask(void *argument);

void App_CreateTasks(void)
{
    AppConfig_t initialConfig;

    g_buttonQueue = xQueueCreate(8U, sizeof(ButtonEvent_t));
    g_sensorQueue = xQueueCreate(1U, sizeof(SensorData_t));
    g_configSaveQueue = xQueueCreate(1U, sizeof(AppConfig_t));
    g_uartRxQueue = xQueueCreate(96U, sizeof(uint8_t));
    g_configMutex = xSemaphoreCreateMutex();
    g_i2cMutex = xSemaphoreCreateMutex();

    if ((g_buttonQueue == NULL) || (g_sensorQueue == NULL) ||
        (g_configSaveQueue == NULL) || (g_uartRxQueue == NULL) ||
        (g_configMutex == NULL) || (g_i2cMutex == NULL))
    {
        Board_AllRelaysOff();
        for (;;) { Board_SetStatusLED(1U); Board_DelayMs(100U); Board_SetStatusLED(0U); Board_DelayMs(100U); }
    }

    I2C1_SetMutex(g_i2cMutex);
    ConfigStore_Load(&initialConfig);
    AppConfig_SetActive(&initialConfig);

    /* Driver initialisation which uses I2C occurs from SensorTask/UITask after scheduler starts. */
    Board_UART1_EnableRxInterrupt();

    (void)xTaskCreate(SensorTask, "Sensor", 320U, NULL, 3U, NULL);
    (void)xTaskCreate(ControlTask, "Control", 320U, NULL, 4U, NULL);
    (void)xTaskCreate(ButtonTask, "Buttons", 160U, NULL, 2U, NULL);
    (void)xTaskCreate(UITask, "OLED_Menu", 320U, NULL, 1U, NULL);
    (void)xTaskCreate(ConfigTask, "Config", 256U, NULL, 2U, NULL);
    (void)xTaskCreate(CommTask, "UART_OTA", 256U, NULL, 2U, NULL);
}

static void SensorTask(void *argument)
{
    SensorData_t data;
    TickType_t lastWake;
    TickType_t lastDht = 0U;
    TickType_t lastLight = 0U;
    TickType_t lastRtc = 0U;
    (void)argument;
    memset(&data, 0, sizeof(data));
    Sensors_Init();
    lastWake = xTaskGetTickCount();
    for (;;)
    {
        TickType_t now = xTaskGetTickCount();
        Sensors_ReadFast(&data);
        if ((now - lastDht) >= pdMS_TO_TICKS(DHT22_PERIOD_MS))
        {
            Sensors_ReadDHT22(&data);
            lastDht = now;
        }
        if ((now - lastLight) >= pdMS_TO_TICKS(BH1750_PERIOD_MS))
        {
            Sensors_ReadLight(&data);
            lastLight = now;
        }
        if ((now - lastRtc) >= pdMS_TO_TICKS(RTC_READ_PERIOD_MS))
        {
            Sensors_ReadRTC(&data);
            lastRtc = now;
        }
        (void)xQueueOverwrite(g_sensorQueue, &data);
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
    }
}

static void pump_set(uint8_t on)
{
    Board_SetPump(on);
    g_pumpOn = on;
}

static void ControlTask(void *argument)
{
    SensorData_t sensor;
    AppConfig_t cfg;
    uint8_t haveSensor = 0U;
    uint8_t pumpMode = 0U; /* 0 off, 1 soil-auto, 2 scheduled, 3 manual */
    TickType_t pumpStarted = 0U;
    TickType_t lastAutoEnd = 0U;
    uint32_t lastScheduleKey = 0xFFFFFFFFUL;
    uint8_t fanState = 0U;
    uint8_t lampState = 0U;
    TickType_t lastWake = xTaskGetTickCount();
    (void)argument;
    memset(&sensor, 0, sizeof(sensor));
    lastAutoEnd = xTaskGetTickCount() - pdMS_TO_TICKS(SOIL_WATER_COOLDOWN_SEC * 1000U);
    pump_set(0U);
    Board_SetFan(0U);
    Board_SetLamp(0U);
    for (;;)
    {
        TickType_t now = xTaskGetTickCount();
        uint8_t thermalDryBlock;
        uint8_t tankOK;
        uint8_t scheduleDue = 0U;
        uint32_t minuteOfDay = 0U;
        uint32_t scheduleKey = 0U;
        if (xQueuePeek(g_sensorQueue, &sensor, 0U) == pdTRUE) haveSensor = 1U;
        AppConfig_GetActive(&cfg);

        if (haveSensor && sensor.rtcValid)
        {
            minuteOfDay = ((uint32_t)sensor.hour * 60UL) + sensor.minute;
            scheduleKey = (sensor.dayKey * 1440UL) + minuteOfDay;
            if (!g_manualMode && (sensor.second <= 5U) && (scheduleKey != lastScheduleKey) &&
                sensor.temperatureValid &&
                ((minuteOfDay == cfg.schedule1Minute) || (minuteOfDay == cfg.schedule2Minute)))
            {
                scheduleDue = 1U;
                lastScheduleKey = scheduleKey; /* one attempt per scheduled minute */
            }
        }

        tankOK = (haveSensor && sensor.waterOK) ? 1U : 0U;
        thermalDryBlock = (haveSensor && sensor.temperatureValid &&
                           (sensor.temperatureC >= (int16_t)cfg.waterBlockTempC) &&
                           (sensor.soilPercent <= (uint8_t)cfg.soilOnPercent)) ? 1U : 0U;
        g_tankLowAlarm = tankOK ? 0U : 1U;

        /* Schedule has priority over the ordinary soil-based cycle. It ignores soil
         * moisture, but tank-low and hot+dry interlocks still stop the pump for safety. */
        if (scheduleDue)
        {
            if (tankOK && !thermalDryBlock && sensor.temperatureValid)
            {
                pumpMode = 2U;
                pumpStarted = now;
                pump_set(1U);
                Board_SetBuzzer(0U);
            }
            else
            {
                /* Marked as attempted, so it will not repeatedly start in this minute. */
                pumpMode = 0U;
                pump_set(0U);
                Board_SetBuzzer(1U);
            }
        }

        if (pumpMode != 0U)
        {
            uint8_t mustStop = 0U;
            if (!tankOK || thermalDryBlock || !sensor.temperatureValid) mustStop = 1U;
            if ((pumpMode == 1U) && (sensor.soilPercent >= (uint8_t)cfg.soilOffPercent)) mustStop = 1U;
            if ((now - pumpStarted) >= pdMS_TO_TICKS(WATERING_PULSE_SECONDS * 1000U)) mustStop = 1U;
            if (mustStop)
            {
                pump_set(0U);
                if (pumpMode == 1U) lastAutoEnd = now;
                if (pumpMode == 3U) g_manualPumpRequest = 0U; /* manual pump is one 10 s pulse */
                pumpMode = 0U;
            }
        }

        if (pumpMode == 0U)
        {
            if (!tankOK || thermalDryBlock || !sensor.temperatureValid)
            {
                pump_set(0U);
                if (g_manualMode) g_manualPumpRequest = 0U;
            }
            else if (g_manualMode && g_manualPumpRequest)
            {
                pumpMode = 3U;
                pumpStarted = now;
                pump_set(1U);
            }
            else if (!g_manualMode && !scheduleDue && (sensor.soilPercent <= (uint8_t)cfg.soilOnPercent) &&
                     ((now - lastAutoEnd) >= pdMS_TO_TICKS(SOIL_WATER_COOLDOWN_SEC * 1000U)))
            {
                pumpMode = 1U;
                pumpStarted = now;
                pump_set(1U);
            }
        }

        /* Fan automatic control uses a 2 C hysteresis to avoid relay chatter. */
        if (g_manualMode)
        {
            fanState = g_manualFanRequest;
            lampState = g_manualLampRequest;
        }
        else
        {
            if (haveSensor && sensor.temperatureValid)
            {
                if (sensor.temperatureC >= (int16_t)cfg.fanOnTempC) fanState = 1U;
                else if (sensor.temperatureC <= ((int16_t)cfg.fanOnTempC - 2)) fanState = 0U;
            }
            if (haveSensor && sensor.lightValid)
            {
                if (sensor.lightLux < cfg.lightOnLux) lampState = 1U;
                else if (sensor.lightLux > (cfg.lightOnLux + 20U)) lampState = 0U;
            }
        }
        Board_SetFan(fanState);
        Board_SetLamp(lampState);
        g_fanOn = fanState;
        g_lampOn = lampState;
        Board_SetStatusLED((g_pumpOn || g_tankLowAlarm) ? 1U : 0U);
        if (g_tankLowAlarm || thermalDryBlock) Board_SetBuzzer(1U);
        else Board_SetBuzzer(0U);
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(CONTROL_TASK_PERIOD_MS));
    }
}

static void ButtonTask(void *argument)
{
    uint8_t previousRaw[4] = {0U,0U,0U,0U};
    uint8_t stableState[4] = {0U,0U,0U,0U};
    uint8_t count[4] = {0U,0U,0U,0U};
    uint8_t i;
    (void)argument;
    for (;;)
    {
        for (i = 0U; i < 4U; i++)
        {
            uint8_t raw = Board_ButtonPressed(i);
            if (raw == previousRaw[i])
            {
                if (count[i] < 3U) count[i]++;
                if ((count[i] >= 3U) && (stableState[i] != raw))
                {
                    stableState[i] = raw;
                    if (raw)
                    {
                        ButtonEvent_t ev = (ButtonEvent_t)i;
                        (void)xQueueSend(g_buttonQueue, &ev, 0U);
                    }
                }
            }
            else
            {
                previousRaw[i] = raw;
                count[i] = 0U;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BUTTON_SCAN_PERIOD_MS));
    }
}

static void ConfigTask(void *argument)
{
    AppConfig_t cfg;
    (void)argument;
    for (;;)
    {
        if (xQueueReceive(g_configSaveQueue, &cfg, portMAX_DELAY) == pdTRUE)
        {
            g_configSaveState = 1U;
            if (ConfigStore_Save(&cfg)) g_configSaveState = 2U;
            else g_configSaveState = 3U;
            vTaskDelay(pdMS_TO_TICKS(1500U));
            g_configSaveState = 0U;
        }
    }
}

#define MENU_ITEM_COUNT 13U
static uint8_t s_menuIndex = 0U;
static uint8_t s_editing = 0U;
static uint8_t s_editingRtc = 0U;
static AppConfig_t s_editConfig;
static DS3231_Time_t s_editRtc;

static void format_time(uint32_t minuteValue, char *out, uint32_t len)
{
    uint32_t h = minuteValue / 60UL;
    uint32_t m = minuteValue % 60UL;
    (void)snprintf(out, len, "%02lu:%02lu", (unsigned long)h, (unsigned long)m);
}

static void edit_adjust(int8_t direction)
{
    if (s_editingRtc)
    {
        if (s_menuIndex == 8U)
        {
            if (direction > 0) s_editRtc.hour = (uint8_t)((s_editRtc.hour + 1U) % 24U);
            else s_editRtc.hour = (s_editRtc.hour == 0U) ? 23U : (uint8_t)(s_editRtc.hour - 1U);
        }
        else if (s_menuIndex == 9U)
        {
            if (direction > 0) s_editRtc.minute = (uint8_t)((s_editRtc.minute + 1U) % 60U);
            else s_editRtc.minute = (s_editRtc.minute == 0U) ? 59U : (uint8_t)(s_editRtc.minute - 1U);
            s_editRtc.second = 0U;
        }
        return;
    }
    switch (s_menuIndex)
    {
        case 1U:
            if (direction > 0 && s_editConfig.soilOnPercent < 70U) s_editConfig.soilOnPercent++;
            if (direction < 0 && s_editConfig.soilOnPercent > 5U) s_editConfig.soilOnPercent--;
            if (s_editConfig.soilOnPercent >= s_editConfig.soilOffPercent)
                s_editConfig.soilOnPercent = s_editConfig.soilOffPercent - 1U;
            break;
        case 2U:
            if (direction > 0 && s_editConfig.soilOffPercent < 95U) s_editConfig.soilOffPercent++;
            if (direction < 0 && s_editConfig.soilOffPercent > 10U) s_editConfig.soilOffPercent--;
            if (s_editConfig.soilOffPercent <= s_editConfig.soilOnPercent)
                s_editConfig.soilOffPercent = s_editConfig.soilOnPercent + 1U;
            break;
        case 3U:
            if (direction > 0 && s_editConfig.fanOnTempC < 50U) s_editConfig.fanOnTempC++;
            if (direction < 0 && s_editConfig.fanOnTempC > 15U) s_editConfig.fanOnTempC--;
            break;
        case 4U:
            if (direction > 0 && s_editConfig.waterBlockTempC < 60U) s_editConfig.waterBlockTempC++;
            if (direction < 0 && s_editConfig.waterBlockTempC > 20U) s_editConfig.waterBlockTempC--;
            break;
        case 5U:
            if (direction > 0 && s_editConfig.lightOnLux < 2000U) s_editConfig.lightOnLux += 10U;
            if (direction < 0 && s_editConfig.lightOnLux >= 10U) s_editConfig.lightOnLux -= 10U;
            break;
        case 6U:
            if (direction > 0) s_editConfig.schedule1Minute = (s_editConfig.schedule1Minute + 1U) % 1440U;
            else s_editConfig.schedule1Minute = (s_editConfig.schedule1Minute == 0U) ? 1439U : s_editConfig.schedule1Minute - 1U;
            break;
        case 7U:
            if (direction > 0) s_editConfig.schedule2Minute = (s_editConfig.schedule2Minute + 1U) % 1440U;
            else s_editConfig.schedule2Minute = (s_editConfig.schedule2Minute == 0U) ? 1439U : s_editConfig.schedule2Minute - 1U;
            break;
        default: break;
    }
}

static void show_menu_item(uint8_t index, const AppConfig_t *cfg, const SensorData_t *sensor)
{
    char line1[20];
    char line2[20];
    char timeText[10];
    const char *name = "";
    switch (index)
    {
        case 0U: name = "MODE"; (void)snprintf(line2, sizeof(line2), "%s", g_manualMode ? "MANUAL" : "AUTO"); break;
        case 1U: name = "SOIL ON"; (void)snprintf(line2, sizeof(line2), "%lu%%", (unsigned long)cfg->soilOnPercent); break;
        case 2U: name = "SOIL OFF"; (void)snprintf(line2, sizeof(line2), "%lu%%", (unsigned long)cfg->soilOffPercent); break;
        case 3U: name = "FAN TEMP"; (void)snprintf(line2, sizeof(line2), "%lu C", (unsigned long)cfg->fanOnTempC); break;
        case 4U: name = "HOT BLOCK"; (void)snprintf(line2, sizeof(line2), "%lu C", (unsigned long)cfg->waterBlockTempC); break;
        case 5U: name = "LIGHT LUX"; (void)snprintf(line2, sizeof(line2), "%lu LUX", (unsigned long)cfg->lightOnLux); break;
        case 6U: name = "SCHEDULE 1"; format_time(cfg->schedule1Minute, timeText, sizeof(timeText)); (void)snprintf(line2, sizeof(line2), "%s", timeText); break;
        case 7U: name = "SCHEDULE 2"; format_time(cfg->schedule2Minute, timeText, sizeof(timeText)); (void)snprintf(line2, sizeof(line2), "%s", timeText); break;
        case 8U:
            name = "RTC HOUR";
            if (s_editingRtc) (void)snprintf(line2, sizeof(line2), "%02u", s_editRtc.hour);
            else if (sensor->rtcValid) (void)snprintf(line2, sizeof(line2), "%02u", sensor->hour);
            else (void)snprintf(line2, sizeof(line2), "NO RTC");
            break;
        case 9U:
            name = "RTC MINUTE";
            if (s_editingRtc) (void)snprintf(line2, sizeof(line2), "%02u", s_editRtc.minute);
            else if (sensor->rtcValid) (void)snprintf(line2, sizeof(line2), "%02u", sensor->minute);
            else (void)snprintf(line2, sizeof(line2), "NO RTC");
            break;
        case 10U: name = "MANUAL FAN"; (void)snprintf(line2, sizeof(line2), "%s", g_manualFanRequest ? "ON" : "OFF"); break;
        case 11U: name = "MANUAL LAMP"; (void)snprintf(line2, sizeof(line2), "%s", g_manualLampRequest ? "ON" : "OFF"); break;
        default: name = "MANUAL PUMP"; (void)snprintf(line2, sizeof(line2), "%s", g_manualPumpRequest ? "ON" : "OFF"); break;
    }
    (void)snprintf(line1, sizeof(line1), "%c%-15s", s_editing ? '*' : '>', name);
    OLED_WriteLine(0U, "MAIN MENU       ");
    OLED_WriteLine(1U, line1);
    OLED_WriteLine(2U, line2);
    OLED_WriteLine(3U, s_editing ? "UP/DOWN CHANGE  " : "UP/DN MOVE      ");
    OLED_WriteLine(4U, s_editing ? "OK SAVE BACK ESC" : "OK SELECT       ");
    if (g_configSaveState == 1U) OLED_WriteLine(6U, "SAVING FLASH    ");
    else if (g_configSaveState == 2U) OLED_WriteLine(6U, "SAVED           ");
    else if (g_configSaveState == 3U) OLED_WriteLine(6U, "SAVE ERROR      ");
    else OLED_WriteLine(6U, "                ");
    OLED_WriteLine(7U, "PA5 OK PB0 BACK  ");
}

static void show_home(const SensorData_t *s, uint8_t page)
{
    char line[20];
    AppConfig_t cfg;
    AppConfig_GetActive(&cfg);
    if (page == 0U)
    {
        OLED_WriteLine(0U, "GREENHOUSE      ");
        (void)snprintf(line, sizeof(line), "SOIL %3u%% W:%s", s->soilPercent, s->waterOK ? "OK" : "LOW"); OLED_WriteLine(1U, line);
        if (s->temperatureValid) (void)snprintf(line, sizeof(line), "TEMP %dC HUM %u%%", (int)s->temperatureC, s->humidityPercent);
        else (void)snprintf(line, sizeof(line), "DHT22 WAIT      "); OLED_WriteLine(2U, line);
        if (s->lightValid) (void)snprintf(line, sizeof(line), "LIGHT %u LUX", s->lightLux);
        else (void)snprintf(line, sizeof(line), "LIGHT SENSOR ERR"); OLED_WriteLine(3U, line);
        (void)snprintf(line, sizeof(line), "P:%s F:%s L:%s", g_pumpOn ? "ON" : "OFF", g_fanOn ? "ON" : "OFF", g_lampOn ? "ON" : "OFF"); OLED_WriteLine(4U, line);
        if (s->rtcValid) (void)snprintf(line, sizeof(line), "TIME %02u:%02u:%02u", s->hour, s->minute, s->second);
        else (void)snprintf(line, sizeof(line), "RTC ERROR       "); OLED_WriteLine(5U, line);
        OLED_WriteLine(6U, g_manualMode ? "MODE MANUAL     " : "MODE AUTO       ");
        if (!s->waterOK) OLED_WriteLine(7U, "ALARM: TANK LOW ");
        else OLED_WriteLine(7U, "OK=MENU UP/DN=PG");
    }
    else
    {
        OLED_WriteLine(0U, "SETTINGS        ");
        (void)snprintf(line, sizeof(line), "SOIL ON %lu%%", (unsigned long)cfg.soilOnPercent); OLED_WriteLine(1U, line);
        (void)snprintf(line, sizeof(line), "SOIL OFF %lu%%", (unsigned long)cfg.soilOffPercent); OLED_WriteLine(2U, line);
        (void)snprintf(line, sizeof(line), "FAN >= %lu C", (unsigned long)cfg.fanOnTempC); OLED_WriteLine(3U, line);
        (void)snprintf(line, sizeof(line), "BLOCK >= %lu C", (unsigned long)cfg.waterBlockTempC); OLED_WriteLine(4U, line);
        (void)snprintf(line, sizeof(line), "LUX ON %lu", (unsigned long)cfg.lightOnLux); OLED_WriteLine(5U, line);
        if (s->rtcValid) (void)snprintf(line, sizeof(line), "S1 %02lu:%02lu S2", (unsigned long)(cfg.schedule1Minute / 60U), (unsigned long)(cfg.schedule1Minute % 60U));
        else (void)snprintf(line, sizeof(line), "RTC NOT READY   "); OLED_WriteLine(6U, line);
        (void)snprintf(line, sizeof(line), "S2 %02lu:%02lu", (unsigned long)(cfg.schedule2Minute / 60U), (unsigned long)(cfg.schedule2Minute % 60U)); OLED_WriteLine(7U, line);
    }
}

static void UITask(void *argument)
{
    ButtonEvent_t event;
    SensorData_t sensor;
    AppConfig_t active;
    uint8_t inMenu = 0U;
    uint8_t homePage = 0U;
    TickType_t lastWake = xTaskGetTickCount();
    (void)argument;
    memset(&sensor, 0, sizeof(sensor));
    OLED_Init();
    for (;;)
    {
        if (xQueuePeek(g_sensorQueue, &sensor, 0U) != pdTRUE) memset(&sensor, 0, sizeof(sensor));
        if (xQueueReceive(g_buttonQueue, &event, 0U) == pdTRUE)
        {
            if (!inMenu)
            {
                if (event == BUTTON_OK) { inMenu = 1U; s_menuIndex = 0U; s_editing = 0U; AppConfig_GetActive(&s_editConfig); }
                else if (event == BUTTON_UP || event == BUTTON_DOWN) homePage ^= 1U;
                else if (event == BUTTON_BACK) homePage = 0U;
            }
            else if (s_editing)
            {
                if (event == BUTTON_UP) edit_adjust(1);
                else if (event == BUTTON_DOWN) edit_adjust(-1);
                else if (event == BUTTON_BACK)
                {
                    s_editing = 0U;
                    s_editingRtc = 0U;
                    AppConfig_GetActive(&s_editConfig);
                }
                else if (event == BUTTON_OK)
                {
                    if (s_editingRtc)
                    {
                        if (DS3231_SetTime(&s_editRtc)) g_configSaveState = 2U;
                        else g_configSaveState = 3U;
                        s_editingRtc = 0U;
                    }
                    else if (AppConfig_Validate(&s_editConfig))
                    {
                        AppConfig_SetActive(&s_editConfig);
                        (void)xQueueOverwrite(g_configSaveQueue, &s_editConfig);
                    }
                    s_editing = 0U;
                }
            }
            else
            {
                if (event == BUTTON_UP) s_menuIndex = (uint8_t)((s_menuIndex + MENU_ITEM_COUNT - 1U) % MENU_ITEM_COUNT);
                else if (event == BUTTON_DOWN) s_menuIndex = (uint8_t)((s_menuIndex + 1U) % MENU_ITEM_COUNT);
                else if (event == BUTTON_BACK) inMenu = 0U;
                else if (event == BUTTON_OK)
                {
                    if (s_menuIndex == 0U) g_manualMode = (uint8_t)!g_manualMode;
                    else if (s_menuIndex >= 1U && s_menuIndex <= 7U)
                    {
                        AppConfig_GetActive(&s_editConfig);
                        s_editingRtc = 0U;
                        s_editing = 1U;
                    }
                    else if ((s_menuIndex == 8U || s_menuIndex == 9U) && sensor.rtcValid)
                    {
                        s_editRtc.second = 0U;
                        s_editRtc.minute = sensor.minute;
                        s_editRtc.hour = sensor.hour;
                        s_editRtc.day = sensor.day;
                        s_editRtc.month = sensor.month;
                        s_editRtc.year = sensor.year;
                        s_editingRtc = 1U;
                        s_editing = 1U;
                    }
                    else if (s_menuIndex == 10U && g_manualMode) g_manualFanRequest = (uint8_t)!g_manualFanRequest;
                    else if (s_menuIndex == 11U && g_manualMode) g_manualLampRequest = (uint8_t)!g_manualLampRequest;
                    else if (s_menuIndex == 12U && g_manualMode) g_manualPumpRequest = (uint8_t)!g_manualPumpRequest;
                }
            }
        }
        if (inMenu)
        {
            AppConfig_GetActive(&active);
            show_menu_item(s_menuIndex, s_editingRtc ? &active : (s_editing ? &s_editConfig : &active), &sensor);
            if (g_manualMode == 0U) { g_manualPumpRequest = 0U; g_manualFanRequest = 0U; g_manualLampRequest = 0U; }
        }
        else
        {
            show_home(&sensor, homePage);
        }
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(DISPLAY_REFRESH_MS));
    }
}

static void CommTask(void *argument)
{
    uint8_t ch;
    char line[24];
    uint8_t pos = 0U;
    TickType_t lastTelemetry = 0U;
    SensorData_t sensor;
    (void)argument;
    Board_UART1_SendString("APP_READY\r\n");
    memset(line, 0, sizeof(line));
    for (;;)
    {
        if (xQueueReceive(g_uartRxQueue, &ch, pdMS_TO_TICKS(100U)) == pdTRUE)
        {
            if (ch == '\r' || ch == '\n')
            {
                line[pos] = '\0';
                if (strcmp(line, "OTA_REQ") == 0 || strcmp(line, "OTA") == 0)
                {
                    Board_AllRelaysOff();
                    Board_SetBuzzer(0U);
                    Board_UART1_SendString("OTA_ACK\r\n");
                    vTaskDelay(pdMS_TO_TICKS(250U));
                    NVIC_SystemReset();
                }
                pos = 0U;
                line[0] = '\0';
            }
            else if (ch >= 32U && ch <= 126U)
            {
                if (pos < (sizeof(line) - 1U)) line[pos++] = (char)ch;
                else { pos = 0U; line[0] = '\0'; }
            }
            else
            {
                /* Binary OTA packet sent while App is active is not a command.
                 * ESP32 should first send OTA_REQ and wait for reset/bootloader. */
                pos = 0U;
                line[0] = '\0';
            }
        }
        if ((xTaskGetTickCount() - lastTelemetry) >= pdMS_TO_TICKS(2000U))
        {
            if (xQueuePeek(g_sensorQueue, &sensor, 0U) == pdTRUE)
            {
                char out[80];
                (void)snprintf(out, sizeof(out), "STAT,SOIL=%u,TEMP=%d,WATER=%u,PUMP=%u\r\n",
                    sensor.soilPercent, (int)sensor.temperatureC, sensor.waterOK, g_pumpOn);
                Board_UART1_SendString(out);
            }
            lastTelemetry = xTaskGetTickCount();
        }
    }
}

/* USART1 RX interrupt: move bytes to a queue; never parse commands in ISR. */
void USART1_IRQHandler(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t value = (uint8_t)USART_ReceiveData(USART1);
        if (g_uartRxQueue != NULL)
            (void)xQueueSendFromISR(g_uartRxQueue, &value, &higherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}
