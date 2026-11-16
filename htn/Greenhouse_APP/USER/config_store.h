#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H
#include <stdint.h>
#include "app_types.h"
void ConfigStore_Load(AppConfig_t *config);
uint8_t ConfigStore_Save(const AppConfig_t *config);
uint32_t ConfigStore_CRC32(const uint8_t *data, uint32_t length);
#endif
