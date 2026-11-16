#include "bh1750.h"
#include "i2c_bus.h"

#define BH1750_ADDR 0x23U

uint8_t BH1750_Init(void)
{
    uint8_t cmd;
    cmd = 0x01U; /* Power on */
    if (!I2C1_WriteBytes(BH1750_ADDR, &cmd, 1U)) return 0U;
    cmd = 0x10U; /* Continuous high-resolution mode */
    return I2C1_WriteBytes(BH1750_ADDR, &cmd, 1U);
}

uint8_t BH1750_ReadLux(uint16_t *lux)
{
    uint8_t raw[2];
    uint16_t value;
    if ((lux == 0) || !I2C1_ReadBytes(BH1750_ADDR, raw, 2U)) return 0U;
    value = (uint16_t)(((uint16_t)raw[0] << 8) | raw[1]);
    *lux = (uint16_t)(value / 1.2f);
    return 1U;
}
