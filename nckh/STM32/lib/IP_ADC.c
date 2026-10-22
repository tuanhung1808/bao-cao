#include "IP_ADC.h"
#include "IP_UART.h"



void ADC_Config(ADC_TypeDef *ADCx, GPIO_TypeDef *GPIOx, int pin) {
    int channel;
    
    // 1. C?p Clock cho GPIO và xác d?nh Channel
    if(GPIOx == GPIOA && pin < 8) {
        RCC->APB2ENR |= 0x04;     // Clock cho GPIOA
        channel = pin;
    }
    else if(GPIOx == GPIOB && pin < 2) {
        RCC->APB2ENR |= 0x08;     // Clock cho GPIOB
        channel = 8 + pin;    
    }
    else if(GPIOx == GPIOC && pin < 6) {
        RCC->APB2ENR |= 0x10;     // Clock cho GPIOC
        channel = 10 + pin;
    }
    
    // 2. C?u hình mode Analog Input (MODE=00, CNF=00)
    if(pin < 8) {
        GPIOx->CRL &= ~(0xF << (pin * 4));
    }
    else if(pin < 16) {
        GPIOx->CRH &= ~(0xF << ((pin - 8) * 4));
    }
    
    // 3. C?p Clock cho ADC và AFIO
    if(ADCx == ADC1) {
        RCC->APB2ENR |= 0x201;    // Clock cho ADC1 và AFIO
    }
    else if(ADCx == ADC2) {
        RCC->APB2ENR |= 0x401;    // Clock cho ADC2 và AFIO
    }
    
    // C?u hình prescaler ADC (Chia 8 d? PCLK2 72MHz -> 9MHz < 14MHz)
    RCC->CFGR |= 0xC000; 
    
    // 4. FIX L?I TIME L?Y M?U: Phân chia SMPR1 và SMPR2
    if (channel < 10) {
        ADCx->SMPR2 |= (7 << (channel * 3));        // Kênh 0 - 9
    } else {
        ADCx->SMPR1 |= (7 << ((channel - 10) * 3)); // Kênh 10 - 17
    }
    
    // 5. C?U HÌNH ADC & HI?U CHU?N
    ADCx->CR2 = 0;              // Reset thanh ghi CR2
    ADCx->CR2 |= 2;             // B?t ch? d? do liên t?c (CONT = 1) ngay t? d?u
//		ADCx->CR2 &= ~2;    // Ð?m b?o CONT = 0, t?t h?n continuous mode
    ADCx->SQR3 = channel;       // M? kênh do d?u tiên
    
    ADCx->CR2 |= 1;             // L?n 1: B?t ngu?n ADC (ADON = 1)
    
    for(int i = 0; i < 5000; i++); // Tr? ch? ADC ?n d?nh (tSTAB)

    // THÊM: Th?c hi?n hi?u chu?n (Calibration)
    ADCx->CR2 |= (1 << 3);              // Reset Calibration
    while(ADCx->CR2 & (1 << 3));        // Ch? reset xong
    ADCx->CR2 |= (1 << 2);              // Start Calibration
    while(ADCx->CR2 & (1 << 2));        // Ch? hi?u chu?n hoàn t?t
    
    ADCx->CR2 |= 1;             // L?n 2: B?t d?u quá trình chuy?n d?i (Start Conversion)
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
   for(int i=0;i<180000;i++);
    ADCx->CR2 |= ADC_CR2_ADON;
		
    ADCx->CR2 |= ADC_CR2_RSTCAL;
    while(ADCx->CR2 & ADC_CR2_RSTCAL);
    ADCx->CR2 |= ADC_CR2_CAL;
    while(ADCx->CR2 & ADC_CR2_CAL);

//	ADCx->CR2 |= 1<<21;          //bat swstart
	  ADCx->CR2 |= ADC_CR2_SWSTART;
   
}


void ADC_DMA_PA4PA5_Conf(void)
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

void ADC_DMA_PA4_5_6_7_Conf(void)
{
	volatile int adc_dma[3];
    // Enable clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_IOPAEN;
    RCC->AHBENR  |= RCC_AHBENR_DMA1EN;

    // PA4,5,6,7 analog mode
    GPIOA->CRL &= ~(0xFFFF << 16);

    // ADC prescaler = PCLK2/6
    RCC->CFGR |= (2 << 14);

    // Scan + Continuous
    ADC1->CR1 |= ADC_CR1_SCAN;
    ADC1->CR2 |= ADC_CR2_CONT;

    // ===== ADC sequence =====
    // 4 conversions => L = 3
    ADC1->SQR1 = (3 << 20);

    // Th? t? kênh: 4 -> 5 -> 6 -> 7
    ADC1->SQR3 = (4 << 0) | (5 << 5) | (6 << 10) | (7 << 15);

    // Sample time (max cho ?n d?nh)
    ADC1->SMPR2 |= (7 << 12) | (7 << 15) | (7 << 18) | (7 << 21);

    // ===== DMA config =====
    DMA1_Channel1->CCR = 0;

    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR  = (uint32_t)adc_dma;
    DMA1_Channel1->CNDTR = 4;

    DMA1_Channel1->CCR = DMA_CCR1_MINC |     // tang d?a ch? RAM
                         DMA_CCR1_MSIZE_0 |  // 16-bit memory
                         DMA_CCR1_PSIZE_0 |  // 16-bit peripheral
                         DMA_CCR1_CIRC;      // vòng l?p

    DMA1_Channel1->CCR |= DMA_CCR1_EN;

    // Enable ADC DMA
    ADC1->CR2 |= ADC_CR2_DMA;

    // ===== Enable ADC =====
    ADC1->CR2 |= ADC_CR2_ADON;
    for(int i = 0; i < 1000; i++);
    ADC1->CR2 |= ADC_CR2_ADON;

    // Reset calibration
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while(ADC1->CR2 & ADC_CR2_RSTCAL);

    // Calibration
    ADC1->CR2 |= ADC_CR2_CAL;
    while(ADC1->CR2 & ADC_CR2_CAL);

    // Start conversion
    ADC1->CR2 |= ADC_CR2_SWSTART;
}



