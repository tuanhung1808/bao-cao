#include "config_store.h"
#include "app_config.h"
#include "stm32f10x_flash.h"
#include <stddef.h>
#include <string.h>

uint32_t ConfigStore_CRC32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t i;
    uint8_t bit;
    for (i = 0U; i < length; i++)
    {
        crc ^= data[i];
        for (bit = 0U; bit < 8U; bit++)
            crc = (crc & 1U) ? ((crc >> 1) ^ 0xEDB88320UL) : (crc >> 1);
    }
    return ~crc;
}

void ConfigStore_Load(AppConfig_t *config)
{
    AppConfig_t saved;
    const volatile uint32_t *src = (const volatile uint32_t *)CONFIG_FLASH_ADDRESS;
    uint32_t *dst = (uint32_t *)&saved;
    uint32_t i;
    for (i = 0U; i < (uint32_t)(sizeof(saved) / sizeof(uint32_t)); i++) dst[i] = src[i];

    if ((saved.magic == APP_CONFIG_MAGIC) && (saved.version == APP_CONFIG_VERSION) &&
        (saved.crc32 == ConfigStore_CRC32((const uint8_t *)&saved, (uint32_t)offsetof(AppConfig_t, crc32))) &&
        AppConfig_Validate(&saved))
    {
        *config = saved;
    }
    else
    {
        AppConfig_Default(config);
        config->crc32 = ConfigStore_CRC32((const uint8_t *)config, (uint32_t)offsetof(AppConfig_t, crc32));
    }
}

uint8_t ConfigStore_Save(const AppConfig_t *config)
{
    AppConfig_t saved;
    uint32_t *words;
    uint32_t i;
    FLASH_Status status;

    if ((config == 0) || !AppConfig_Validate(config)) return 0U;
    saved = *config;
    saved.crc32 = ConfigStore_CRC32((const uint8_t *)&saved, (uint32_t)offsetof(AppConfig_t, crc32));

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    status = FLASH_ErasePage(CONFIG_FLASH_ADDRESS);
    if (status != FLASH_COMPLETE)
    {
        FLASH_Lock();
        return 0U;
    }

    words = (uint32_t *)&saved;
    for (i = 0U; i < (uint32_t)(sizeof(saved) / sizeof(uint32_t)); i++)
    {
        status = FLASH_ProgramWord(CONFIG_FLASH_ADDRESS + (i * 4U), words[i]);
        if (status != FLASH_COMPLETE)
        {
            FLASH_Lock();
            return 0U;
        }
    }
    FLASH_Lock();

    /* Read-back verification prevents showing "saved" if flash programming failed. */
    for (i = 0U; i < (uint32_t)(sizeof(saved) / sizeof(uint32_t)); i++)
        if (((const volatile uint32_t *)CONFIG_FLASH_ADDRESS)[i] != words[i]) return 0U;
    return 1U;
}
