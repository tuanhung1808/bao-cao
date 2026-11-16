#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

/* STM32F103xE / 128 KB Flash layout used by the group's bootloader.
 * App starts at page 50 (0x0800C800); metadata and user config occupy pages 126/127.
 */
#define APP_FLASH_BASE             0x08000000UL
#define APP_START_ADDRESS          0x0800C800UL
#define APP_FLASH_LIMIT            0x0801F800UL
#define CONFIG_FLASH_ADDRESS       0x0801FC00UL
#define CONFIG_FLASH_PAGE_SIZE     1024UL

/* ADC calibration: use debugger to observe raw values and tune these constants.
 * On the common analog soil board, raw usually decreases as soil gets wetter.
 * Water strip analog output usually increases when more of the strip is wet.
 */
#define SOIL_RAW_DRY               3200U
#define SOIL_RAW_WET               1400U
#define WATER_RAW_MIN              800U

#define RELAY_ACTIVE_LOW           1
#define BUZZER_ACTIVE_HIGH         1
#define STATUS_LED_ACTIVE_LOW      1

#define WATERING_PULSE_SECONDS     10U
#define SOIL_WATER_COOLDOWN_SEC    60U

#define DEFAULT_SOIL_ON_PERCENT    35U
#define DEFAULT_SOIL_OFF_PERCENT   60U
#define DEFAULT_FAN_TEMP_C         32U
#define DEFAULT_WATER_BLOCK_TEMP_C 38U
#define DEFAULT_LIGHT_ON_LUX       200U
#define DEFAULT_SCHEDULE_1_MIN     (7U * 60U)  /* 07:00 */
#define DEFAULT_SCHEDULE_2_MIN     (17U * 60U) /* 17:00 */

#define SENSOR_TASK_PERIOD_MS      500U
#define RTC_READ_PERIOD_MS         1000U
#define DHT22_PERIOD_MS            2000U
#define BH1750_PERIOD_MS           1000U
#define CONTROL_TASK_PERIOD_MS      100U
#define BUTTON_SCAN_PERIOD_MS        10U
#define DISPLAY_REFRESH_MS          300U

#endif
