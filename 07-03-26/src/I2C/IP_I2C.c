#include "IP_I2C.h"
#include "IP_GPIO.h"


void I2C_Config(I2C_TypeDef *I2Cx){
	if(I2Cx==I2C1){
		RCC->APB1ENR |=0x200000;    //mo clock cho i2c1
		IP_GPIO_outPut_AF_OpenDrain_50Mhz(GPIOB,6);
		IP_GPIO_outPut_AF_OpenDrain_50Mhz(GPIOB,7);
	}
	else {
		RCC->APB1ENR |= 0x400000;    //mo clock cho i2c2
		IP_GPIO_outPut_AF_OpenDrain_50Mhz(GPIOB,10);
		IP_GPIO_outPut_AF_OpenDrain_50Mhz(GPIOB,11);
	}
   	I2C1->CR1 |= 0x8000;          //RESET peripheral i2c
		I2C1->CR1 &= ~0x8000;
	
	  I2C1->CR1 = 0;                // Reset
    I2C1->CR2 = 36;               // APB1 = 36MHz

    I2C1->CCR = 180;              // 100kHz cho scl
    I2C1->TRISE = 37;             // (36MHz + 1)

    I2C1->CR1 |= I2C_CR1_PE;      // Enable peripheral I2C
	
}






