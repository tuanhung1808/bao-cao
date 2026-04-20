#include "IP_ADC.h"
#include "IP_GPIO.h"

void ADC_Config(ADC_TypeDef *ADCx, GPIO_TypeDef *GPIOx, int pin) {
	int channel;
	if(GPIOx==GPIOA && pin<8){
	  RCC->APB2ENR |= 0x04; //clock cho gpio
		channel=pin;
	}
	else if(GPIOx == GPIOB && pin <2){
		RCC->APB2ENR |= 0x08; //clock cho gpio
		channel= 8 +pin;	
	}
	else if(GPIOx==GPIOC && pin<6){
		RCC->APB2ENR |= 0x10;
		channel = 10 + pin;
	}
	
//mode analog input		
	if( pin < 8 ){
	GPIOx->CRL &= ~(0xF << (pin*4));
}
else if( pin < 16 ){
	GPIOx->CRH &= ~(0xF << ((pin-8)*4));
}
//		IP_GPIO_Input_Analog(GPIOx, pin); 
	
	if(ADCx == ADC1){
		RCC->APB2ENR |= 0x201; //clock cho adc, AF
	}
	else if(ADCx == ADC2){
		RCC->APB2ENR |= 0x401; //clock cho adc, AF
	}
		RCC->CFGR |= 0x8000; //tan so chia 8 de <14MHz
	
	  ADCx->SMPR2 |= (7 << (channel*3));	
  	ADCx->CR2 = 0;        //reset ADON
		ADCx->SQR3 = channel; //mo kenh
		ADCx->CR2 |= 1;
		delay(10);            //cac bit trong ADON k dc thay doi at the same time
		ADCx->CR2 |= 1;
		ADCx->CR2 |= 2;       //CONT
	
}


int ADC_Check(ADC_TypeDef *ADCx, GPIO_TypeDef *GPIOx, int pin){
	int check=0;
	if(ADCx->SR & 2){  //mo EOC(1 khi conversion hoan thanh, 0 khi chua hoan thanh)
		check =1;
	}
	return check;
}

int ADC_Rx(ADC_TypeDef *ADCx, GPIO_TypeDef *GPIOx, int pin){

  int result=0;
  int data=0;
	ADCx->CR2 |= (1<<22);          // Start conversion (SWSTART)	
	while(!(ADCx->SR & (1<<1)));   // Doi EOC
	data = ADCx->DR;
  result = data;	
return result;
}

void ADC_Channel(ADC_TypeDef *ADCx, GPIO_TypeDef *GPIOx, int pin, int rank){
		int channel;
	if(GPIOx==GPIOA && pin<8){
	  RCC->APB2ENR |= 0x04; //clock cho gpio
		channel=pin;
	}
	else if(GPIOx == GPIOB && pin <2){
		RCC->APB2ENR |= 0x08; //clock cho gpio
		channel= 8 +pin;	
	}
	else if(GPIOx==GPIOC && pin<6){
		RCC->APB2ENR |= 0x10;
		channel = 10 + pin;
	}
	
//mode analog input		
	if( pin < 8 ){
	GPIOx->CRL &= ~(0xF << (pin*4));
}
else if( pin < 16 ){
	GPIOx->CRH &= ~(0xF << ((pin-8)*4));
}

//do uu tien cua kenh
	  ADCx->SMPR2 |= (7 << (channel*3));	
		ADCx->SQR3 |= (channel << (rank-1)*5) ; //do uu tien cua kenh

}



void ADC_DMA_Init(ADC_TypeDef *ADCx ,int sokenh){
	volatile uint32_t data[sokenh];

	RCC->AHBENR |= 1<<0;     //clock cho DMA;
	
		if(ADCx == ADC1){
		RCC->APB2ENR |= 0x201; //clock cho adc, AF
	}
	else if(ADCx == ADC2){
		RCC->APB2ENR |= 0x401; //clock cho adc, AF
	}
	
	RCC->CFGR |= 0x8000;   //tan so chia 8
//	ADCx->CR1 |= 1<<8 ;    //bat scan mode
//	ADCx->CR2 |= 1<<1;        //bat che do continue
	  ADCx->CR1 |= ADC_CR1_SCAN;
    ADCx->CR2 |= ADC_CR2_CONT;
	
	ADCx->SQR1 |= (sokenh-1) << 20;   //so luong kenh
	
	
	DMA1_Channel1->CCR = 0;   //tat channel thi moi cau hinh duoc
	DMA1_Channel1->CPAR = (uint32_t)&ADCx->DR;  //dia chi peripheral
	DMA1_Channel1->CMAR =(uint32_t)data;     //dia chi memtory, du lieu dc chuyen vao day
	DMA1_Channel1->CNDTR = sokenh;    //so luong phan tu
	DMA1_Channel1->CCR = DMA_CCR1_MINC | DMA_CCR1_MSIZE_0 |
                         DMA_CCR1_PSIZE_0 | DMA_CCR1_CIRC;
  DMA1_Channel1->CCR |= DMA_CCR1_EN;
	
//  ADCx -> CR2  |= 1<<8;        //bat dma cho adc     
	ADCx->CR2 |= ADC_CR2_DMA;

//		ADCx-> CR2 |= 1<<0;          //bat ADON
//		delay(10);
//		ADCx->CR2 |= 1<<0;           //bat lai ADON 
	  ADCx->CR2 |= ADC_CR2_ADON;
   delay(100);
    ADCx->CR2 |= ADC_CR2_ADON;
		
    ADCx->CR2 |= ADC_CR2_RSTCAL;
    while(ADCx->CR2 & ADC_CR2_RSTCAL);
    ADCx->CR2 |= ADC_CR2_CAL;
    while(ADCx->CR2 & ADC_CR2_CAL);

//	ADCx->CR2 |= 1<<21;          //bat swstart
	  ADCx->CR2 |= ADC_CR2_SWSTART;
   
}


void ADC_DMA_Conf(void)
{
    volatile	uint16_t adc_dma[2];
	
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_IOPAEN;
    RCC->AHBENR  |= RCC_AHBENR_DMA1EN;

    ADC1->SQR1 = (1 << 20);

    // ADC prescaler
    RCC->CFGR |= (2 << 14);

    // Scan + Continuous
    ADC1->CR1 |= ADC_CR1_SCAN;
    ADC1->CR2 |= ADC_CR2_CONT;



    // DMA
    DMA1_Channel1->CCR = 0;
    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR  = (uint32_t)adc_dma;
    DMA1_Channel1->CNDTR = 2;
    DMA1_Channel1->CCR = DMA_CCR1_MINC | DMA_CCR1_MSIZE_0 |
                         DMA_CCR1_PSIZE_0 | DMA_CCR1_CIRC;
    DMA1_Channel1->CCR |= DMA_CCR1_EN;

    ADC1->CR2 |= ADC_CR2_DMA;

    // Enable ADC
    ADC1->CR2 |= ADC_CR2_ADON;
    for(int i=0;i<1000;i++);
    ADC1->CR2 |= ADC_CR2_ADON;

    // Calibrate
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while(ADC1->CR2 & ADC_CR2_RSTCAL);
    ADC1->CR2 |= ADC_CR2_CAL;
    while(ADC1->CR2 & ADC_CR2_CAL);

    // Start
    ADC1->CR2 |= ADC_CR2_SWSTART;
		
		
		
		
	    // PA4, PA5 analog
    GPIOA->CRL &= ~(0xFF << 16);	
		
		    // 2 conversions

    ADC1->SQR3 = (4 << 0) | (5 << 5);

    ADC1->SMPR2 |= (7 << 12) | (7 << 15);
		
}

