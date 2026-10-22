#include "stm32f10x.h"
#include "GPIO.h"
#include "PWM.h"
#include "BUTTON.h"
#include "SYSTICK.h"
#include "PID.h"
#include "motor_config.h"
#include "motor_protocol.h"
#include <stdio.h>

#define RING_SIZE 128u
#define RING_MASK (RING_SIZE - 1u)
static volatile uint8_t rx_ring[RING_SIZE], tx_ring[RING_SIZE];
static volatile uint8_t rx_head, rx_tail, tx_head, tx_tail, rx_fault;
static uint8_t direction = 1, reversing, stopped_samples;
static uint32_t reverse_started;
static MP_Receiver receiver;

static void USART3_Init_9600(void)
{
    uint32_t code = (RCC->CFGR >> 8) & 7u;
    uint32_t div = code < 4u ? 1u : (1u << (code - 3u));
    uint32_t pclk = SystemCoreClock / div;
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    GPIOB->CRH &= ~((0xFu << 8) | (0xFu << 12));
    GPIOB->CRH |= (0xBu << 8) | (0x4u << 12); /* PB10 TX, PB11 RX. */
    USART3->CR1 = 0;
    USART3->CR2 = 0;
    USART3->CR3 = 1u;
    USART3->BRR = (pclk + 4800u) / 9600u;
    USART3->CR1 = (1u << 13) | (1u << 5) | (1u << 3) | (1u << 2);
    NVIC_EnableIRQ(USART3_IRQn);
}
void USART3_IRQHandler(void)
{
    uint32_t sr = USART3->SR;
    if (sr & ((1u << 5) | 0x0Fu)) {
        uint8_t c = (uint8_t)USART3->DR; /* SR then DR clears RX/error flags. */
        if (sr & 0x0Fu) rx_fault = 1;
        else {
            uint8_t next = (uint8_t)((rx_head + 1u) & RING_MASK);
            if (next == rx_tail) rx_fault = 1;
            else { rx_ring[rx_head] = c; rx_head = next; }
        }
    }
    if ((sr & (1u << 7)) && (USART3->CR1 & (1u << 7))) {
        if (tx_tail != tx_head) {
            USART3->DR = tx_ring[tx_tail];
            tx_tail = (uint8_t)((tx_tail + 1u) & RING_MASK);
        } else USART3->CR1 &= ~(1u << 7);
    }
}
static int UART_GetByte(char *c)
{
    uint32_t key = __get_PRIMASK();
    __disable_irq();
    if (rx_fault) { rx_tail = rx_head; rx_fault = 0; MP_Reset(&receiver); }
    if (rx_tail == rx_head) { __set_PRIMASK(key); return 0; }
    *c = (char)rx_ring[rx_tail];
    rx_tail = (uint8_t)((rx_tail + 1u) & RING_MASK);
    __set_PRIMASK(key);
    return 1;
}
/* Queue a complete frame or skip it; never wait on UART inside the PID loop. */
static void UART_Queue(const char *p, size_t n)
{
    uint32_t key = __get_PRIMASK();
    __disable_irq();
    unsigned available = (tx_tail - tx_head - 1u) & RING_MASK;
    if (n <= available) {
        while (n--) {
            tx_ring[tx_head] = (uint8_t)*p++;
            tx_head = (uint8_t)((tx_head + 1u) & RING_MASK);
        }
        USART3->CR1 |= 1u << 7;
    }
    __set_PRIMASK(key);
}
static void Motor_Direction(uint8_t forward)
{
    GPIOA->BSRR = forward ? ((1u << 9) | (1u << 26)) : ((1u << 10) | (1u << 25));
}
static void Motor_Stop(void) { TIM3->CCR1 = 0; PID_Reset(&Motor_PID); }
int main(void)
{
    /* SystemInit was already called by startup. */
    SysTick_Init();
    GPIO_Config_Button();
    GPIO_Config_PWM();
    PWM_config();
    Encoder_Init();
    USART3_Init_9600();
    Motor_PID.Out_Max = (float)TIM3->ARR;
    Motor_PID.FeedforwardGain = MOTOR_FEEDFORWARD_RPM > 0.0f ? Motor_PID.Out_Max / MOTOR_FEEDFORWARD_RPM : 0.0f;
    Motor_PID.Setpoint = 0.0f;
    Motor_Direction(direction);
    Motor_Stop();
    uint16_t requested_rpm = 0;
    uint8_t have_command = 0;
    uint32_t last_command = millis(), last_control = millis(), last_report = millis();
    uint16_t previous_count = (uint16_t)TIM2->CNT;
    uint32_t sample_counts = 0;
    float filtered_rpm = 0.0f;
    for (;;) {
        char c;
        while (UART_GetByte(&c)) {
            if (MP_Feed(&receiver, c)) {
                uint32_t value;
                if (MP_Parse(receiver.line, 'S', &value, 1) && value <= MOTOR_MAX_RPM) {
                    requested_rpm = (uint16_t)value;
                    last_command = millis();
                    have_command = 1;
                }
            }
        }
        uint32_t now = millis();
        uint8_t linked = have_command && ((uint32_t)(now - last_command) <= COMMAND_TIMEOUT_MS);
        if (!linked) requested_rpm = 0;
        Motor_PID.Setpoint = (float)requested_rpm;
        Button_Update(&Btn_Rev);
        if (Button_Read_Event(&Btn_Rev)) {
            reversing = !reversing; /* Second press cancels a pending reversal. */
            reverse_started = now;
            stopped_samples = 0;
            Motor_Stop();
        }
        if (!linked) reversing = 0;
        if (!linked || !requested_rpm || reversing) Motor_Stop();
        uint32_t elapsed = now - last_control;
        if (elapsed >= CONTROL_PERIOD_MS) {
            last_control = now;
            uint16_t count = (uint16_t)TIM2->CNT;
            int32_t delta = (int16_t)(uint16_t)(count - previous_count);
            previous_count = count;
            sample_counts = (uint32_t)(delta < 0 ? -delta : delta);
            float rpm = (float)sample_counts * 60000.0f / (ENCODER_COUNTS_PER_REV * (float)elapsed);
            float dt = elapsed * 0.001f;
            filtered_rpm += dt / (SPEED_FILTER_TAU_S + dt) * (rpm - filtered_rpm);
            if (!sample_counts && filtered_rpm < 0.05f) filtered_rpm = 0.0f;
            if (reversing) {
                if (rpm < REVERSE_STOP_RPM) {
                    if (stopped_samples < REVERSE_STABLE_SAMPLES) ++stopped_samples;
                } else stopped_samples = 0;
                if ((uint32_t)(now - reverse_started) >= REVERSE_COAST_MS && stopped_samples >= REVERSE_STABLE_SAMPLES) {
                    direction = !direction;
                    Motor_Direction(direction);
                    reversing = 0;
                    PID_Reset(&Motor_PID);
                }
            }
            if (linked && requested_rpm && !reversing)
                TIM3->CCR1 = (uint16_t)PID_Compute(&Motor_PID, filtered_rpm);
            else Motor_Stop();
        }
        if ((uint32_t)(now - last_report) >= TELEMETRY_PERIOD_MS) {
            last_report = now;
            unsigned state = !linked ? 3u : reversing ? 2u : requested_rpm ? 1u : 0u;
            unsigned duty = (unsigned)(((uint32_t)TIM3->CCR1 * 100u + TIM3->ARR / 2u) / (TIM3->ARR + 1u));
            float report_rpm = filtered_rpm > 99999.0f ? 99999.0f : filtered_rpm;
            char payload[56], frame[MP_CAPACITY];
            int n = snprintf(payload, sizeof(payload), "T,%u,%lu,%u,%u,%u,%lu",
                (unsigned)requested_rpm, (unsigned long)(report_rpm * 10.0f + 0.5f),
                duty, (unsigned)direction, state, (unsigned long)sample_counts);
            if (n > 0 && (size_t)n < sizeof(payload)) {
                size_t len = MP_Encode(frame, sizeof(frame), payload);
                if (len) UART_Queue(frame, len);
            }
        }
    }
}
