#include "IP_TIMER.h"
#include "IP_GPIO.h"

void TIMER_Configure(uint16_t pin, TIM_TypeDef *TIMx){
	if(pin <=7 && pin >=2){	
	RCC -> APB1ENR  |=(1<<(pin-2));
	}
	else if(pin==1){
		RCC->APB2ENR |= 1<< 11;
	}
	TIMx -> ARR      = 0xFFFF; //thanh ghi tu dong reload
	TIMx -> PSC      = 72 - 1;
	TIMx -> CR1     |= 0x01;
	TIMx -> EGR     |= 0x01;	
	
	}

void delay_ms(int time, TIM_TypeDef *TIMx){
	while(time){
		TIMx -> CNT      = 0;
		while((TIMx -> CNT ) < 1000){
		}
		time--;
	}
}

void delay_s(int time, TIM_TypeDef *TIMx){
	while(time*1000){
		TIMx -> CNT      = 0;
		while((TIMx -> CNT ) < 1000){
		}
		time--;
	}
}

void delay_us(int time, TIM_TypeDef *TIMx){
	while(time){
		TIM2 -> CNT      = 0;
		while((TIMx -> CNT ) < 1){
		}
		time--;
	}
}

void TIMER_PWM(uint32_t SpeedDongco1, uint32_t SpeedDongco2, uint32_t SpeedDongco3, uint32_t SpeedDongco4){
RCC->APB1ENR |= 0x01;
	
	TIM2-> ARR |= 10000-1;
	TIM2->PSC |= 7200 -1;
	TIM2->CCMR1 |= 0x6060; //0110 : pwm mode
	TIM2->CCMR2 |= 0x6060;
	
	TIM2->CCR1 = (SpeedDongco1*10000)/100;
	TIM2->CCR2 = (SpeedDongco2*10000)/100;
	TIM2->CCR3 = (SpeedDongco3*10000)/100;
	TIM2->CCR4 = (SpeedDongco4*10000)/100;
	
	TIM2->CCER |= 0x1111; //upcounter
	TIM2->CR1 = 0x01; //enable CEN
	TIM2->EGR |= 0x01; //enable UG
	
}	

void TIMER_PWM_Servo(GPIO_TypeDef *GPIOx, int pin, int speed)
{
    TIM_TypeDef *tim = 0;
    int channel = 0;

    /* TIM1 */
    if(GPIOx==GPIOA && pin>=8 && pin<=11){
        RCC->APB2ENR |= (1<<11);
        tim = TIM1;
        channel = pin - 7;
    }
    /* TIM2 */
    else if(GPIOx==GPIOA && pin>=0 && pin<=3){
        RCC->APB1ENR |= (1<<0);
        tim = TIM2;
        channel = pin + 1;
    }
    /* TIM3 */
    else if((GPIOx==GPIOA && (pin==6 || pin==7)) ||
            (GPIOx==GPIOB && (pin==0 || pin==1))){
        RCC->APB1ENR |= (1<<1);
        tim = TIM3;
        channel = (GPIOx==GPIOA) ? (pin-5) : (pin+3);
    }
    /* TIM4 */
    else if(GPIOx==GPIOB && pin>=6 && pin<=9){
        RCC->APB1ENR |= (1<<2);
        tim = TIM4;
        channel = pin - 5;
    }
    else return;

		IP_GPIO_outPut_AF_OpenDrain_50Mhz(GPIOx,pin);
		
    tim->PSC = 7200 - 1;
    tim->ARR = 200 - 1;

    switch(channel){
        case 1: tim->CCMR1 |= (6<<4);   break;
        case 2: tim->CCMR1 |= (6<<12);  break;
        case 3: tim->CCMR2 |= (6<<4);   break;
        case 4: tim->CCMR2 |= (6<<12);  break;
    }

    tim->CCER |= (1 << ((channel-1)*4));  //mo capture compare cho cac kenh

    if(tim == TIM1)
        tim->BDTR |= (1<<15);

    switch(channel){
        case 1: tim->CCR1 = 1000+(speed*1000)/180; break;
        case 2: tim->CCR2 = 1000+(speed*1000)/180; break;
        case 3: tim->CCR3 = 1000+(speed*1000)/180; break;
        case 4: tim->CCR4 = 1000+(speed*1000)/180; break;
    }
tim->EGR |= 1;
    tim->CR1 |= 1;
	
}

void TIMER_PWM_Led(GPIO_TypeDef *GPIOx, int pin, int speed){
	if(speed < 0) speed = 0;
	else if(speed > 100) speed =100;
	
	TIM_TypeDef *tim ;
	int channel;
	
	    if(GPIOx==GPIOA && pin>=8 && pin<=11){
        RCC->APB2ENR |= (1<<11);
        tim = TIM1;
        channel = pin - 7;
    }
    /* TIM2 */
    else if(GPIOx==GPIOA && pin>=0 && pin<=3){
        RCC->APB1ENR |= (1<<0);
        tim = TIM2;
        channel = pin + 1;
    }
    /* TIM3 */
    else if((GPIOx==GPIOA && (pin==6 || pin==7)) ||
            (GPIOx==GPIOB && (pin==0 || pin==1))){
        RCC->APB1ENR |= (1<<1);
        tim = TIM3;
        channel = (GPIOx==GPIOA) ? (pin-5) : (pin+3);
    }
    /* TIM4 */
    else if(GPIOx==GPIOB && pin>=6 && pin<=9){
        RCC->APB1ENR |= (1<<2);
        tim = TIM4;
        channel = pin - 5;
    }
    else return;
		
			IP_GPIO_outPut_AF_OpenDrain_50Mhz(GPIOx,pin);
		
    tim->PSC = 72 - 1;
    tim->ARR = 1000 - 1;

    switch(channel){
        case 1: tim->CCMR1 |= (6<<4);   break;
        case 2: tim->CCMR1 |= (6<<12);  break;
        case 3: tim->CCMR2 |= (6<<4);   break;
        case 4: tim->CCMR2 |= (6<<12);  break;
    }

    tim->CCER |= (1 << ((channel-1)*4));  //mo capture compare cho cac kenh

    if(tim == TIM1)
        tim->BDTR |= (1<<15);

    switch(channel){
        case 1: tim->CCR1 = (speed*1000)/100; break;
        case 2: tim->CCR2 = (speed*1000)/100; break;
        case 3: tim->CCR3 = (speed*1000)/100; break;
        case 4: tim->CCR4 = (speed*1000)/100; break;
    }
tim->EGR |= 1;
    tim->CR1 |= 1;
	
}



