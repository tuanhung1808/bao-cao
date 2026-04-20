#ifndef __IP_GPIO_H
#include "stm32f10x.h"

void IP_GPIO_outPut_Low(GPIO_TypeDef* GPIOx, uint16_t pinNumber);
void IP_GPIO_inPut_Low(GPIO_TypeDef* GPIOx, uint16_t pinNumber);
void setBit(GPIO_TypeDef* GPIOx, uint16_t pinNumber);
void reSetBit(GPIO_TypeDef* GPIOx, uint16_t pinNumber);
int IP_ReadData(GPIO_TypeDef* GPIOx, uint16_t pinNumber);
void IP_Toggle(GPIO_TypeDef* GPIOx, uint16_t pinNumberIn,GPIO_TypeDef* GPIOy, uint16_t pinNumberOut);
void delay(uint32_t ms);
void IP_GPIO_outPut_AF_OpenDrain_50Mhz(GPIO_TypeDef *GPIOx, uint16_t pinNumber);
void IP_GPIO_Input_Analog(GPIO_TypeDef *GPIOx, int pinNumber);



#endif


