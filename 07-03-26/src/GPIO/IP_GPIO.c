#include "IP_GPIO.h" 

void delay(uint32_t ms){
	  for(uint32_t i = 0; i < ms * 8000; i++);
}

void IP_GPIO_outPut_Low(GPIO_TypeDef *GPIOx, uint16_t pinNumber){
if(GPIOx == GPIOA){
RCC->APB2ENR |= 0x04;
}
else if(GPIOx == GPIOB){
RCC->APB2ENR |= 0x08;
}
else if(GPIOx == GPIOC){
	RCC->APB2ENR |= 0x10;
}

if( pinNumber < 8 ){
	GPIOx->CRL &= ~(0xF << (pinNumber * 4));
	GPIOx->CRL |= 3 << (pinNumber*4);
}
else if( pinNumber < 16 ){
	GPIOx->CRH &= ~(0xF << ((pinNumber -8) * 4));
	GPIOx->CRH |= 3 << ((pinNumber-8)*4);
}
}

void IP_GPIO_outPut_AF_OpenDrain_50Mhz(GPIO_TypeDef *GPIOx, uint16_t pinNumber){
if(GPIOx == GPIOA){
RCC->APB2ENR |= 0x04;
}
else if(GPIOx == GPIOB){
RCC->APB2ENR |= 0x08;
}
else if(GPIOx == GPIOC){
	RCC->APB2ENR |= 0x10;
}

if( pinNumber < 8 ){
	GPIOx->CRL &= ~(0xF << (pinNumber * 4));
	GPIOx->CRL |= 0xB << (pinNumber*4);
}
else if( pinNumber < 16 ){
	GPIOx->CRH &= ~(0xF << ((pinNumber -8) * 4));
	GPIOx->CRH |= 0xB << ((pinNumber-8)*4);
}
}

void setBit(GPIO_TypeDef* GPIOx, uint16_t pinNumber){
      GPIOx->BSRR = (1 << pinNumber);  
}
void reSetBit(GPIO_TypeDef* GPIOx, uint16_t pinNumber){
	    GPIOx->BRR = (1 << pinNumber);    
}

void IP_GPIO_inPut_Low(GPIO_TypeDef* GPIOx, uint16_t pinNumber){
	if(GPIOx == GPIOA){
	RCC->APB2ENR |= 0x04;
	}
	else if(GPIOx == GPIOB){
		RCC->APB2ENR |= 0x08;
	}
	else if(GPIOx == GPIOC){
		RCC->APB2ENR |= 0x10;
	}
	if(pinNumber < 8){
		GPIOx->CRL &= ~(0xF << (pinNumber * 4));
		GPIOx->CRL |= 8 << pinNumber*4;
	}
	if(pinNumber < 16){
		GPIOx->CRH &= ~(0xF << ((pinNumber -8) * 4));
		GPIOx->CRH |= 8 << ((pinNumber-8)*4);
	}	
}

int IP_ReadData(GPIO_TypeDef* GPIOx, uint16_t pinNumber){
if((GPIOx->IDR & (1 << pinNumber)) == 0){
return 0;
}
else {
	return 1;
}
}
void IP_Toggle(GPIO_TypeDef* GPIOx, uint16_t pinNumberIn,GPIO_TypeDef* GPIOy, uint16_t pinNumberOut){
	uint8_t last = 0;
GPIOy->ODR &=~(1<< pinNumberOut);
	
while (1)
{
    uint8_t now = (GPIOx->IDR & (1 << pinNumberIn)) ? 1 : 0;
	delay(10);

    if (last == 0 && now == 1){
        GPIOy->ODR ^= (1 << pinNumberOut);
		delay(10);
		}
			
    last = now;
}

}
void IP_GPIO_InPut_Analog(GPIO_TypeDef *GPIOx, int pinNumber){
	if(GPIOx == GPIOA){
RCC->APB2ENR |= 0x04;
}
else if(GPIOx == GPIOB){
RCC->APB2ENR |= 0x08;
}
else if(GPIOx == GPIOC){
	RCC->APB2ENR |= 0x10;
}

if( pinNumber < 8 ){
	GPIOx->CRL &= ~(0xF << (pinNumber*4));
}
else if( pinNumber < 16 ){
	GPIOx->CRH &= ~(0xF << ((pinNumber-8)*4));
}
}


