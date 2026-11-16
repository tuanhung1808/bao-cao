#include "i2c_bus.h"
#include "stm32f10x.h"
#include "stm32f10x_i2c.h"
#include "task.h"

#define I2C_TIMEOUT_COUNT  60000UL
static SemaphoreHandle_t s_i2cMutex = NULL;

void I2C1_SetMutex(SemaphoreHandle_t mutex) { s_i2cMutex = mutex; }

static uint8_t bus_lock(void)
{
    if ((s_i2cMutex != NULL) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
        return (xSemaphoreTake(s_i2cMutex, pdMS_TO_TICKS(200)) == pdTRUE) ? 1U : 0U;
    return 1U;
}
static void bus_unlock(void)
{
    if ((s_i2cMutex != NULL) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
        (void)xSemaphoreGive(s_i2cMutex);
}
static uint8_t wait_flag(I2C_TypeDef *i2c, uint32_t flag, FlagStatus state)
{
    uint32_t n = I2C_TIMEOUT_COUNT;
    while (I2C_GetFlagStatus(i2c, flag) != state)
    {
        if (n-- == 0U) return 0U;
    }
    return 1U;
}
static void stop_bus(void)
{
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
}

uint8_t I2C1_WriteBytes(uint8_t address7, const uint8_t *data, uint16_t length)
{
    uint16_t i;
    uint8_t ok = 0U;
    if ((data == 0) || (length == 0U) || (address7 > 0x7FU) || !bus_lock()) return 0U;

    if (!wait_flag(I2C1, I2C_FLAG_BUSY, RESET)) goto done;
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!wait_flag(I2C1, I2C_FLAG_SB, SET)) goto done;
    I2C_Send7bitAddress(I2C1, (uint8_t)(address7 << 1), I2C_Direction_Transmitter);
    if (!wait_flag(I2C1, I2C_FLAG_ADDR, SET)) goto done;
    (void)I2C_ReadRegister(I2C1, I2C_Register_SR1);
    (void)I2C_ReadRegister(I2C1, I2C_Register_SR2);

    for (i = 0U; i < length; i++)
    {
        if (!wait_flag(I2C1, I2C_FLAG_TXE, SET)) goto done;
        I2C_SendData(I2C1, data[i]);
    }
    if (!wait_flag(I2C1, I2C_FLAG_BTF, SET)) goto done;
    ok = 1U;

done:
    stop_bus();
    bus_unlock();
    return ok;
}

uint8_t I2C1_ReadRegisters(uint8_t address7, uint8_t reg, uint8_t *data, uint16_t length)
{
    uint16_t remaining;
    uint8_t ok = 0U;
    if ((data == 0) || (length == 0U) || (address7 > 0x7FU) || !bus_lock()) return 0U;

    I2C_AcknowledgeConfig(I2C1, ENABLE);
    if (!wait_flag(I2C1, I2C_FLAG_BUSY, RESET)) goto done;
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!wait_flag(I2C1, I2C_FLAG_SB, SET)) goto done;
    I2C_Send7bitAddress(I2C1, (uint8_t)(address7 << 1), I2C_Direction_Transmitter);
    if (!wait_flag(I2C1, I2C_FLAG_ADDR, SET)) goto done;
    (void)I2C_ReadRegister(I2C1, I2C_Register_SR1);
    (void)I2C_ReadRegister(I2C1, I2C_Register_SR2);
    if (!wait_flag(I2C1, I2C_FLAG_TXE, SET)) goto done;
    I2C_SendData(I2C1, reg);
    if (!wait_flag(I2C1, I2C_FLAG_BTF, SET)) goto done;

    I2C_GenerateSTART(I2C1, ENABLE);
    if (!wait_flag(I2C1, I2C_FLAG_SB, SET)) goto done;
    I2C_Send7bitAddress(I2C1, (uint8_t)(address7 << 1), I2C_Direction_Receiver);
    if (!wait_flag(I2C1, I2C_FLAG_ADDR, SET)) goto done;

    if (length == 1U)
    {
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR1);
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR2);
        I2C_GenerateSTOP(I2C1, ENABLE);
        if (!wait_flag(I2C1, I2C_FLAG_RXNE, SET)) goto done;
        data[0] = (uint8_t)I2C_ReceiveData(I2C1);
    }
    else if (length == 2U)
    {
        /* STM32F1 two-byte receive sequence: POS=Next and ACK=0 before
         * clearing ADDR, then wait for BTF and issue STOP before reading DR. */
        I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Next);
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR1);
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR2);
        if (!wait_flag(I2C1, I2C_FLAG_BTF, SET)) goto done;
        I2C_GenerateSTOP(I2C1, ENABLE);
        data[0] = (uint8_t)I2C_ReceiveData(I2C1);
        data[1] = (uint8_t)I2C_ReceiveData(I2C1);
        I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);
    }
    else
    {
        I2C_AcknowledgeConfig(I2C1, ENABLE);
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR1);
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR2);
        remaining = length;
        while (remaining > 3U)
        {
            if (!wait_flag(I2C1, I2C_FLAG_RXNE, SET)) goto done;
            *data++ = (uint8_t)I2C_ReceiveData(I2C1);
            remaining--;
        }
        if (!wait_flag(I2C1, I2C_FLAG_BTF, SET)) goto done;
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        *data++ = (uint8_t)I2C_ReceiveData(I2C1);
        if (!wait_flag(I2C1, I2C_FLAG_BTF, SET)) goto done;
        I2C_GenerateSTOP(I2C1, ENABLE);
        *data++ = (uint8_t)I2C_ReceiveData(I2C1);
        *data = (uint8_t)I2C_ReceiveData(I2C1);
    }
    ok = 1U;

done:
    I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    if (ok == 0U) I2C_GenerateSTOP(I2C1, ENABLE);
    bus_unlock();
    return ok;
}


/* Read-only transaction for devices (such as BH1750) that do not use a
 * register pointer before returning data. */
uint8_t I2C1_ReadBytes(uint8_t address7, uint8_t *data, uint16_t length)
{
    uint16_t remaining;
    uint8_t ok = 0U;
    uint8_t *p = data;
    if ((data == 0) || (length == 0U) || (address7 > 0x7FU) || !bus_lock()) return 0U;
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    if (!wait_flag(I2C1, I2C_FLAG_BUSY, RESET)) goto done;
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!wait_flag(I2C1, I2C_FLAG_SB, SET)) goto done;
    I2C_Send7bitAddress(I2C1, (uint8_t)(address7 << 1), I2C_Direction_Receiver);
    if (!wait_flag(I2C1, I2C_FLAG_ADDR, SET)) goto done;
    if (length == 1U)
    {
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR1);
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR2);
        I2C_GenerateSTOP(I2C1, ENABLE);
        if (!wait_flag(I2C1, I2C_FLAG_RXNE, SET)) goto done;
        p[0] = (uint8_t)I2C_ReceiveData(I2C1);
    }
    else if (length == 2U)
    {
        I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Next);
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR1);
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR2);
        if (!wait_flag(I2C1, I2C_FLAG_BTF, SET)) goto done;
        I2C_GenerateSTOP(I2C1, ENABLE);
        p[0] = (uint8_t)I2C_ReceiveData(I2C1);
        p[1] = (uint8_t)I2C_ReceiveData(I2C1);
        I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);
    }
    else
    {
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR1);
        (void)I2C_ReadRegister(I2C1, I2C_Register_SR2);
        remaining = length;
        while (remaining > 3U)
        {
            if (!wait_flag(I2C1, I2C_FLAG_RXNE, SET)) goto done;
            *p++ = (uint8_t)I2C_ReceiveData(I2C1);
            remaining--;
        }
        if (!wait_flag(I2C1, I2C_FLAG_BTF, SET)) goto done;
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        *p++ = (uint8_t)I2C_ReceiveData(I2C1);
        if (!wait_flag(I2C1, I2C_FLAG_BTF, SET)) goto done;
        I2C_GenerateSTOP(I2C1, ENABLE);
        *p++ = (uint8_t)I2C_ReceiveData(I2C1);
        *p = (uint8_t)I2C_ReceiveData(I2C1);
    }
    ok = 1U;
done:
    I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    if (ok == 0U) I2C_GenerateSTOP(I2C1, ENABLE);
    bus_unlock();
    return ok;
}
