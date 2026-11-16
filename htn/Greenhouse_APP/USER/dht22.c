#include "dht22.h"
#include "board.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "FreeRTOS.h"
#include "task.h"

#define DHT_PIN GPIO_Pin_3
#define DHT_PORT GPIOA

static uint8_t wait_level(BitAction level, uint16_t timeoutUs)
{
    uint16_t start = (uint16_t)TIM_GetCounter(TIM2);
    while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) != (BitAction)(level == Bit_SET ? Bit_SET : Bit_RESET))
    {
        if ((uint16_t)((uint16_t)TIM_GetCounter(TIM2) - start) >= timeoutUs) return 0U;
    }
    return 1U;
}

static void dht_input_pullup(void)
{
    GPIO_InitTypeDef g;
    g.GPIO_Pin = DHT_PIN;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT_PORT, &g);
}
static void dht_output_od(void)
{
    GPIO_InitTypeDef g;
    g.GPIO_Pin = DHT_PIN;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    g.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(DHT_PORT, &g);
}

uint8_t DHT22_Read(int16_t *temperatureC, uint8_t *humidityPercent)
{
    uint8_t bytes[5] = {0U, 0U, 0U, 0U, 0U};
    uint8_t i, bit;
    uint16_t highWidth;
    uint16_t start;
    uint16_t humidity10;
    uint16_t temperature10;
    uint8_t sum;

    if ((temperatureC == 0) || (humidityPercent == 0)) return 0U;

    dht_output_od();
    GPIO_ResetBits(DHT_PORT, DHT_PIN);
    Board_DelayUs(1100U);
    GPIO_SetBits(DHT_PORT, DHT_PIN);
    Board_DelayUs(30U);
    dht_input_pullup();

    /* DHT22 protocol is timing-sensitive. Keep this short read critical so an
     * RTOS tick does not distort the 26-70 us data pulses. It blocks interrupts
     * for only a few milliseconds every two seconds. */
    taskENTER_CRITICAL();
    if (!wait_level(Bit_RESET, 100U)) { taskEXIT_CRITICAL(); return 0U; }
    if (!wait_level(Bit_SET, 100U))   { taskEXIT_CRITICAL(); return 0U; }
    if (!wait_level(Bit_RESET, 100U)) { taskEXIT_CRITICAL(); return 0U; }

    for (i = 0U; i < 40U; i++)
    {
        if (!wait_level(Bit_SET, 100U)) { taskEXIT_CRITICAL(); return 0U; }
        start = (uint16_t)TIM_GetCounter(TIM2);
        while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_SET)
        {
            if ((uint16_t)((uint16_t)TIM_GetCounter(TIM2) - start) > 110U) break;
        }
        highWidth = (uint16_t)((uint16_t)TIM_GetCounter(TIM2) - start);
        bit = (highWidth > 45U) ? 1U : 0U;
        bytes[i / 8U] <<= 1;
        bytes[i / 8U] |= bit;
        if (!wait_level(Bit_RESET, 100U)) { taskEXIT_CRITICAL(); return 0U; }
    }
    taskEXIT_CRITICAL();

    sum = (uint8_t)(bytes[0] + bytes[1] + bytes[2] + bytes[3]);
    if (sum != bytes[4]) return 0U;

    humidity10 = (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
    temperature10 = (uint16_t)(((uint16_t)(bytes[2] & 0x7FU) << 8) | bytes[3]);
    *humidityPercent = (uint8_t)(humidity10 / 10U);
    if (bytes[2] & 0x80U) *temperatureC = (int16_t)-(temperature10 / 10U);
    else *temperatureC = (int16_t)(temperature10 / 10U);
    if (*humidityPercent > 100U || *temperatureC < -40 || *temperatureC > 80) return 0U;
    return 1U;
}
