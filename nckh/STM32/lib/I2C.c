#include "I2C.h"
#include "SYSTICK.h"

void I2C_Init(void){
    I2C2->CR1 |= (1<<15);
    delay_ms(10);
    I2C2->CR1 &= ~(1<<15);  // kich hoat swrst dua toan bo thanh ghi I2C ve trang thai mac dinh

    I2C2->CR1 &= ~(1<<0);   // PE = 0 // tam thoi tat PE de cau hinh clock

    /* C?u hình Clock chu?n cho STM32 ch?y 8MHz (APB1 = 8MHz) */
//    I2C2->CR2 = 8;          // APB1 = 8MHz (Ðã s?a t? 36 -> 8)
//    I2C2->CCR = 40;         // Standard mode 100kHz: 8MHz / (2 * 100kHz) = 40 (Ðã s?a t? 180 -> 40)
//    I2C2->TRISE = 9;        // Thoi gian tang xung max (1000ns) voi clock 8MHz: 8 + 1 = 9 (Ðã s?a t? 37 -> 9)
I2C2->CR2 = 36;
I2C2->CCR = 180;
I2C2->TRISE = 37;
	
	
	
    I2C2->CR1 |= (1<<10);   // ACK enable cho phep gui tin hieu ACK sau moi byte nhan 
    I2C2->CR1 |= (1<<0);    // PE = 1
}

void I2C_Start(void){
    I2C2->CR1 |= (1<<8);          // START
    while(!(I2C2->SR1 & (1<<0))); // Cho co SB (Start Bit)
}

//void I2C_Start(void){
//    uint32_t timeout = 100000;
//    while(I2C2->SR2 & (1<<1)){   // Ch? c? BUSY = 0 (bus dã r?nh h?n)
//        if(--timeout == 0) return;   // Tránh treo vinh vi?n n?u có s? c? khác
//    }

//    I2C2->CR1 |= (1<<8);          // START
//    while(!(I2C2->SR1 & (1<<0))); // Ch? c? SB
//}

void I2C_Stop(void){
    I2C2->CR1 |= (1<<9);          // tao tin hieu stop
}

void I2C_Send_Addr(uint8_t addr, uint8_t rw){
    I2C2->DR = (uint16_t)((addr<<1) | (rw & 0x01));
    while(!(I2C2->SR1 & (1<<1))){
        // Cho co ADDR
    }
    // trong stm32 sau khi gui dia chi thanh cong de xoa co SR1 phai doc sr1 va sr2 
    (void)I2C2->SR1;
    (void)I2C2->SR2;
}
//uint8_t I2C_Send_Addr(uint8_t addr, uint8_t rw){
//    I2C2->DR = (uint16_t)((addr<<1) | (rw & 0x01));
//    uint32_t timeout = 100000;
//    while(!(I2C2->SR1 & (1<<1))){
//        if(I2C2->SR1 & (1<<10)){          // AF set = ch?c ch?n b? NACK
//            I2C2->SR1 &= ~(uint32_t)(1<<10);
//            I2C_Stop();
//            return 1;                      // Xác nh?n NACK, không treo n?a
//        }
//        if(--timeout == 0){
//            I2C_Stop();
//            return 2;                      // Timeout khác, không ph?i NACK
//        }
//    }
//    (void)I2C2->SR1;
//    (void)I2C2->SR2;
//    return 0;
//}




uint8_t I2C_Send_Data(uint8_t data){
    I2C2->DR = data;
    while(!(I2C2->SR1 & ((1<<2)|(1<<10)))){
        // Cho co BTF (Byte Transfer Finished) hoac AF (Acknowledge Failure)
    }
    if(I2C2->SR1 & (1<<10)){
        I2C2->SR1 &= ~(uint32_t)(1<<10); // Xoa co AF
        I2C_Stop();                      // dung truyen vi slave dang ko phan hoi 
        return 1;                        // Loi
    }
    return 0;                            // 0 la truyen thanh cong
}

uint8_t I2C_Read_Data(uint8_t ack){
    if(ack){
        I2C2->CR1 |= (1<<10);   // ack=1; bao la van muon nhan them byte 
    }
    else{
        I2C2->CR1 &= ~(uint32_t)(1<<10);
        I2C2->CR1 |= (uint32_t)(1<<9);
    }
    while(!(I2C2->SR1 & (1<<6))){
        // Cho co RxNE (Data register not empty)
    }
    return (uint8_t)I2C2->DR;
}

void ConfigI2C(void){
    RCC->APB1ENR |= (1<<22);    // I2C2 clock enable
    RCC->APB2ENR |= (1<<3);     // GPIOB clock enable
    RCC->APB2ENR |= (1<<0);     // AFIO clock enable
    
    // Cau hinh PB6 (SCL) va PB7 (SDA) la Alternate Function Open Drain (AFOD)
    GPIOB->CRH &= ~(uint32_t)(0xFFFFFFFF);
    GPIOB->CRH |= (0x0000FF00);
	
	    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH &= ~(0xF << 20);
    GPIOC->CRH |= (0x3 << 20);
    GPIOC->ODR =  0x0;
	
	 delay_ms(1000); 
}
