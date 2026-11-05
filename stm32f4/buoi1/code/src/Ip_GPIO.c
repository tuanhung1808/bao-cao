#include "Ip_GPIO.h"

void Ip_GPIO_Configuration(GPIO_TypeDef *GPIOx, uint32_t PinSelect)
{
if(GPIOx == GPIOA)
{
    RCC->AHB1ENR |= 0x01; // Enable clock for GPIOA

}
else if(GPIOx == GPIOB)
{
    RCC->AHB1ENR |= 0x02; // Enable clock for GPIOB
}
else if(GPIOx == GPIOC)
{
    RCC->AHB1ENR |= 0x04; // Enable clock for GPIOC
}
else if(GPIOx == GPIOD)
{
    RCC->AHB1ENR |= 0x08; // Enable clock for GPIOD
}
else if(GPIOx == GPIOE)
{
    RCC->AHB1ENR |= 0x10; // Enable clock for GPIOE
}
else if(GPIOx == GPIOF)
{
    RCC->AHB1ENR |= 0x20; // Enable clock for GPIOF
}
else if(GPIOx == GPIOG)
{
    RCC->AHB1ENR |= 0x40; // Enable clock for GPIOG
}
else if(GPIOx == GPIOH)
{
    RCC->AHB1ENR |= 0x80; // Enable clock for GPIOH
}
else if(GPIOx == GPIOI)
{
    RCC->AHB1ENR |= 0x100; // Enable clock for GPIOI
}

GPIOx->Mode |= 1 << 2*PinSelect; // Set the mode for the selected pins
GPIOx->OTYPER &= 0 << PinSelect; // Set output type to push-pull for the selected pins
GPIOx->OSPEEDR |= 3 << 2*PinSelect; // Set output speed to high for the selected pins
GPIOx->PUPDR |= 1 << 2*PinSelect; // Set no pull-up/pull-down for the selected pins


}