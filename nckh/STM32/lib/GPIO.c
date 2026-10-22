#include "GPIO.h"

void GPIO_Config_Button(void)
{
    /* Enable GPIOA */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /* PA2 PA3 PA4 : Input Pull-up */
    GPIOA->CRL &= ~((0xF<<8)|(0xF<<12)|(0xF<<16));
    GPIOA->CRL |=  ((0x8<<8)|(0x8<<12)|(0x8<<16));

    GPIOA->ODR |= (1<<2)|(1<<3)|(1<<4);

    /* PA9 PA10 : Output Push Pull 50MHz */
    GPIOA->CRH &= ~((0xF<<4)|(0xF<<8));
    GPIOA->CRH |=  ((0x3<<4)|(0x3<<8));
}

void GPIO_Config_PWM(void)
{
    /* GPIOA + AFIO + TIM3 */
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* PA6 -> TIM3_CH1 AF PushPull */
    GPIOA->CRL &= ~(0xF<<24);
    GPIOA->CRL |=  (0xB<<24);
}

void Encoder_Init(void)
{
    /* GPIOA + TIM2 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* PA0 PA1 Floating Input */
    GPIOA->CRL &= ~((0xF<<0)|(0xF<<4));
    GPIOA->CRL |=  ((0x4<<0)|(0x4<<4));

    TIM2->CR1 = 0;
    TIM2->PSC = 0;
    TIM2->ARR = 0xFFFF;
    TIM2->CNT = 0;

    /* CC1 <- TI1
       CC2 <- TI2 */
    TIM2->CCMR1 = 0;
    TIM2->CCMR1 |= (1<<0);
    TIM2->CCMR1 |= (1<<8);

    /* Rising edge */
    TIM2->CCER = 0;

    /* Encoder mode 3 */
    TIM2->SMCR &= ~(7<<0);
    TIM2->SMCR |=  (3<<0);
//TIM2->SMCR |= (2<<0);
    TIM2->CR1 |= TIM_CR1_CEN;
}
