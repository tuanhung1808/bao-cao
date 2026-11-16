#include "ds3231.h"
#include "i2c_bus.h"

#define DS3231_ADDR 0x68U
static uint8_t bcd_to_dec(uint8_t v) { return (uint8_t)(((v >> 4) * 10U) + (v & 0x0FU)); }
static uint8_t dec_to_bcd(uint8_t v) { return (uint8_t)(((v / 10U) << 4) | (v % 10U)); }

uint8_t DS3231_ReadTime(DS3231_Time_t *t)
{
    uint8_t r[7];
    if ((t == 0) || !I2C1_ReadRegisters(DS3231_ADDR, 0x00U, r, 7U)) return 0U;
    t->second = bcd_to_dec((uint8_t)(r[0] & 0x7FU));
    t->minute = bcd_to_dec((uint8_t)(r[1] & 0x7FU));
    if (r[2] & 0x40U)
    {
        uint8_t hour12 = bcd_to_dec((uint8_t)(r[2] & 0x1FU));
        uint8_t isPM = (uint8_t)((r[2] & 0x20U) != 0U);
        t->hour = (uint8_t)((hour12 % 12U) + (isPM ? 12U : 0U));
    }
    else t->hour = bcd_to_dec((uint8_t)(r[2] & 0x3FU));
    t->day = bcd_to_dec((uint8_t)(r[4] & 0x3FU));
    t->month = bcd_to_dec((uint8_t)(r[5] & 0x1FU));
    t->year = (uint16_t)(2000U + bcd_to_dec(r[6]));
    if ((t->second > 59U) || (t->minute > 59U) || (t->hour > 23U) ||
        (t->day < 1U) || (t->day > 31U) || (t->month < 1U) || (t->month > 12U)) return 0U;
    return 1U;
}

uint8_t DS3231_SetTime(const DS3231_Time_t *t)
{
    uint8_t d[8];
    if (t == 0 || t->second > 59U || t->minute > 59U || t->hour > 23U ||
        t->day < 1U || t->day > 31U || t->month < 1U || t->month > 12U ||
        t->year < 2000U || t->year > 2099U) return 0U;
    d[0] = 0x00U;
    d[1] = dec_to_bcd(t->second);
    d[2] = dec_to_bcd(t->minute);
    d[3] = dec_to_bcd(t->hour);
    d[4] = 1U; /* Day of week: 1..7, can be adjusted separately if needed */
    d[5] = dec_to_bcd(t->day);
    d[6] = dec_to_bcd(t->month);
    d[7] = dec_to_bcd((uint8_t)(t->year - 2000U));
    return I2C1_WriteBytes(DS3231_ADDR, d, 8U);
}
