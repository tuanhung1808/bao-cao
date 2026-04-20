#ifndef __IP_ADC_H
#include "stm32f10x.h"


void ADC_Config(ADC_TypeDef *ADCx, GPIO_TypeDef *GPIOx, int pin);
int ADC_Check(ADC_TypeDef *ADCx, GPIO_TypeDef *GPIOx, int pin);
int ADC_Rx(ADC_TypeDef *ADCx, GPIO_TypeDef *GPIOx, int pin);
void ADC_Channel(ADC_TypeDef *ADCx, GPIO_TypeDef *GPIOx, int pin, int rank);
void ADC_DMA_Init(ADC_TypeDef *ADCx ,int sokenh);
void ADC_DMA_Conf(void);

#endif
