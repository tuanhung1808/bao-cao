#include "board.h"
#include "app_config.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"
#include "misc.h"

static volatile uint16_t s_adcBuffer[2];

static void gpio_output(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef g;
    g.GPIO_Pin = pin;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(port, &g);
}

void Board_Init(void)
{
    GPIO_InitTypeDef g;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* Safe state first: all relay channels OFF before starting tasks. */
    GPIO_SetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
    gpio_output(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
    gpio_output(GPIOB, GPIO_Pin_1); /* buzzer */
    gpio_output(GPIOC, GPIO_Pin_13); /* onboard status LED */
    Board_AllRelaysOff();
    Board_SetBuzzer(0U);
    Board_SetStatusLED(0U);

    /* Buttons: PA5=OK/MENU, PA6=UP, PA7=DOWN, PB0=BACK; active LOW. */
    g.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &g);
    g.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOB, &g);

    /* DHT22 on PA3; set to open-drain idle high. */
    g.GPIO_Pin = GPIO_Pin_3;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    g.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(GPIOA, &g);
    GPIO_SetBits(GPIOA, GPIO_Pin_3);

    SystemCoreClockUpdate();
    Board_TIM2_Init1MHz();
}

void Board_TIM2_Init1MHz(void)
{
    TIM_TimeBaseInitTypeDef t;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseStructInit(&t);
    t.TIM_Prescaler = (uint16_t)((SystemCoreClock / 1000000UL) - 1UL);
    t.TIM_CounterMode = TIM_CounterMode_Up;
    t.TIM_Period = 0xFFFFU;
    t.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &t);
    TIM_SetCounter(TIM2, 0U);
    TIM_Cmd(TIM2, ENABLE);
}

void Board_DelayUs(uint16_t us)
{
    uint16_t start = (uint16_t)TIM_GetCounter(TIM2);
    while ((uint16_t)((uint16_t)TIM_GetCounter(TIM2) - start) < us) { }
}

void Board_DelayMs(uint32_t ms)
{
    /* Only used before scheduler startup. During operation use vTaskDelay(). */
    volatile uint32_t i;
    while (ms-- != 0U)
    {
        for (i = 0U; i < (SystemCoreClock / 8000U); i++) { __NOP(); }
    }
}

uint8_t Board_ButtonPressed(uint8_t index)
{
    uint8_t state = 1U;
    switch (index)
    {
        case 0U: state = (uint8_t)GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5); break; /* OK */
        case 1U: state = (uint8_t)GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6); break; /* UP */
        case 2U: state = (uint8_t)GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7); break; /* DOWN */
        default: state = (uint8_t)GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0); break; /* BACK */
    }
    return (state == 0U) ? 1U : 0U;
}

static void relay_write(uint16_t pin, uint8_t on)
{
#if RELAY_ACTIVE_LOW
    if (on) GPIO_ResetBits(GPIOB, pin); else GPIO_SetBits(GPIOB, pin);
#else
    if (on) GPIO_SetBits(GPIOB, pin); else GPIO_ResetBits(GPIOB, pin);
#endif
}

/* Relay mapping from the latest wiring diagram (both 2-channel modules active LOW):
 * PB12 = IN1 = fan, PB13 = IN2 = lamp, PB14 = IN3 = water pump, PB15 = IN4 spare.
 */
void Board_SetFan(uint8_t on)   { relay_write(GPIO_Pin_12, on); }
void Board_SetLamp(uint8_t on)  { relay_write(GPIO_Pin_13, on); }
void Board_SetPump(uint8_t on)  { relay_write(GPIO_Pin_14, on); }
void Board_SetRelay4(uint8_t on){ relay_write(GPIO_Pin_15, on); }

void Board_AllRelaysOff(void)
{
    relay_write(GPIO_Pin_12, 0U);
    relay_write(GPIO_Pin_13, 0U);
    relay_write(GPIO_Pin_14, 0U);
    relay_write(GPIO_Pin_15, 0U);
}

void Board_SetBuzzer(uint8_t on)
{
#if BUZZER_ACTIVE_HIGH
    if (on) GPIO_SetBits(GPIOB, GPIO_Pin_1); else GPIO_ResetBits(GPIOB, GPIO_Pin_1);
#else
    if (on) GPIO_ResetBits(GPIOB, GPIO_Pin_1); else GPIO_SetBits(GPIOB, GPIO_Pin_1);
#endif
}

void Board_SetStatusLED(uint8_t on)
{
#if STATUS_LED_ACTIVE_LOW
    if (on) GPIO_ResetBits(GPIOC, GPIO_Pin_13); else GPIO_SetBits(GPIOC, GPIO_Pin_13);
#else
    if (on) GPIO_SetBits(GPIOC, GPIO_Pin_13); else GPIO_ResetBits(GPIOC, GPIO_Pin_13);
#endif
}

void Board_ADC_Init(void)
{
    GPIO_InitTypeDef g;
    ADC_InitTypeDef a;
    DMA_InitTypeDef d;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); /* 72/6 = 12 MHz ADC clock */

    g.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2;
    g.GPIO_Mode = GPIO_Mode_AIN;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &g);

    DMA_DeInit(DMA1_Channel1);
    DMA_StructInit(&d);
    d.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    d.DMA_MemoryBaseAddr = (uint32_t)s_adcBuffer;
    d.DMA_DIR = DMA_DIR_PeripheralSRC;
    d.DMA_BufferSize = 2U;
    d.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    d.DMA_MemoryInc = DMA_MemoryInc_Enable;
    d.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    d.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    d.DMA_Mode = DMA_Mode_Circular;
    d.DMA_Priority = DMA_Priority_High;
    d.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &d);
    DMA_Cmd(DMA1_Channel1, ENABLE);

    ADC_StructInit(&a);
    a.ADC_Mode = ADC_Mode_Independent;
    a.ADC_ScanConvMode = ENABLE;
    a.ADC_ContinuousConvMode = ENABLE;
    a.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    a.ADC_DataAlign = ADC_DataAlign_Right;
    a.ADC_NbrOfChannel = 2U;
    ADC_Init(ADC1, &a);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1U, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 2U, ADC_SampleTime_55Cycles5);
    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) != RESET) { }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) != RESET) { }
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

uint8_t Board_ADC_Read(uint8_t index, uint16_t *value)
{
    if ((value == 0) || (index > 1U)) return 0U;
    *value = s_adcBuffer[index];
    return 1U;
}

void Board_I2C1_Init(void)
{
    GPIO_InitTypeDef g;
    I2C_InitTypeDef i;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    g.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    g.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &g);

    I2C_DeInit(I2C1);
    I2C_StructInit(&i);
    i.I2C_ClockSpeed = 100000U;
    i.I2C_Mode = I2C_Mode_I2C;
    i.I2C_DutyCycle = I2C_DutyCycle_2;
    i.I2C_OwnAddress1 = 0x00U;
    i.I2C_Ack = I2C_Ack_Enable;
    i.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &i);
    I2C_Cmd(I2C1, ENABLE);
}

void Board_UART1_Init(uint32_t baud)
{
    GPIO_InitTypeDef g;
    USART_InitTypeDef u;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    g.GPIO_Pin = GPIO_Pin_9;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    g.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &g);
    g.GPIO_Pin = GPIO_Pin_10;
    g.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &g);

    USART_StructInit(&u);
    u.USART_BaudRate = baud;
    u.USART_WordLength = USART_WordLength_8b;
    u.USART_StopBits = USART_StopBits_1;
    u.USART_Parity = USART_Parity_No;
    u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    u.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &u);
    USART_Cmd(USART1, ENABLE);
}

void Board_UART1_EnableRxInterrupt(void)
{
    NVIC_InitTypeDef n;
    USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    n.NVIC_IRQChannel = USART1_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 6U; /* FreeRTOS-safe priority */
    n.NVIC_IRQChannelSubPriority = 0U;
    n.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&n);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

void Board_UART1_SendString(const char *s)
{
    while (*s != '\0')
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) { }
        USART_SendData(USART1, (uint16_t)(uint8_t)*s++);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) { }
}
