#include "LCD.h"
#define LCD_ADDR  0x27   



#define RS  (1 << 0)
#define RW  (1 << 1)     
#define EN  (1 << 2)
#define BL  (1 << 3)    



static void LCD_Write(uint8_t data){ 
    I2C_Start();
	GPIOC->ODR ^= (1<<13); delay_ms(1000);  
	
    I2C_Send_Addr(LCD_ADDR, 0);   
	GPIOC->ODR ^= (1<<13); delay_ms(1000);  
	
    I2C_Send_Data(data | BL);    
	GPIOC->ODR ^= (1<<13); delay_ms(1000);  
	
    I2C_Stop();
	GPIOC->ODR ^= (1<<13); delay_ms(1000);  
	
}

//static void LCD_Write(uint8_t data){
//    I2C_Start();
////    GPIOC->ODR ^= (1<<13); delay_ms(1000);   // Debug A: qua Start

//    uint8_t err = I2C_Send_Addr(LCD_ADDR, 0);
//    
////    if(err == 1){
////        // NACK th?t - nháy nhanh 3 l?n liên t?c d? báo hi?u riêng
////        for(int i=0; i<3; i++){
////            GPIOC->ODR ^= (1<<13); delay_ms(250);
////            GPIOC->ODR ^= (1<<13); delay_ms(250);
////        }
////        while(1);   // D?ng h?n t?i dây d? d? nh?n bi?t, không ch?y ti?p
////    }
////    else if(err == 2){
////        // Timeout khác NACK - nháy nhanh 5 l?n liên t?c
////        for(int i=0; i<5; i++){
////            GPIOC->ODR ^= (1<<13); delay_ms(250);
////            GPIOC->ODR ^= (1<<13); delay_ms(250);
////        }
////        while(1);
////    }

////    GPIOC->ODR ^= (1<<13); delay_ms(1000);   // Debug B: qua Send_Addr (thành công th?t s?)

//    I2C_Send_Data(data | BL);
////    GPIOC->ODR ^= (1<<13); delay_ms(1000);   // Debug C

//    I2C_Stop();
////    GPIOC->ODR ^= (1<<13); delay_ms(1000);   // Debug D
//}


static void LCD_Pulse(uint8_t data){//tao xung EN
    LCD_Write(data | EN);
   delay_ms(50);
    LCD_Write(data & ~EN);
      delay_ms(50);
}


static void LCD_Send4Bit(uint8_t data){
    LCD_Pulse(data);
}


static void LCD_SendCmd(uint8_t cmd){
    uint8_t high = (cmd & 0xF0) | BL;
    uint8_t low  = ((cmd << 4) & 0xF0) | BL;

    LCD_Send4Bit(high);
    LCD_Send4Bit(low);
}


void LCD_SendData(uint8_t data){
    uint8_t high = (data & 0xF0) | RS | BL;
    uint8_t low  = ((data << 4) & 0xF0) | RS | BL;

    LCD_Send4Bit(high);
    LCD_Send4Bit(low);
}



void LCD_Init(void){
     delay_ms(5);   

   
    LCD_Send4Bit(0x30);
    delay_ms(5);
    LCD_Send4Bit(0x30);
   delay_ms(5);
    LCD_Send4Bit(0x30);
    delay_ms(5);
    LCD_Send4Bit(0x20); 

   
    LCD_SendCmd(0x28);  // 4-bit, 2 dòng
    LCD_SendCmd(0x0C);  
    LCD_SendCmd(0x06);  // entry mode
    LCD_SendCmd(0x01);  // clear
    delay_ms(5);
}

//		void LCD_Init(void){


//			delay_ms(5);

//			LCD_Send4Bit(0x30);
//			GPIOC->ODR ^= (1<<13);  // Debug 1
//			delay_ms(1000);

//			LCD_Send4Bit(0x30);
//			GPIOC->ODR ^= (1<<13);  // Debug 2
//			delay_ms(1000);

//			LCD_Send4Bit(0x30);
//			GPIOC->ODR ^= (1<<13);  // Debug 3
//			delay_ms(1000);

//			LCD_Send4Bit(0x20);
//			GPIOC->ODR ^= (1<<13);  // Debug 4
//		delay_ms(1000);

//			LCD_SendCmd(0x28);
//			GPIOC->ODR ^= (1<<13);  // Debug 5
//		delay_ms(1000);

//			LCD_SendCmd(0x0C);
//			GPIOC->ODR ^= (1<<13);  // Debug 6
//		delay_ms(1000);

//			LCD_SendCmd(0x06);
//			GPIOC->ODR ^= (1<<13);  // Debug 7
//		delay_ms(1000);

//			LCD_SendCmd(0x01);
//			GPIOC->ODR ^= (1<<13);  // Debug 8
//		delay_ms(1000);

//			delay_ms(5);
//		}


void LCD_Clear(void){
    LCD_SendCmd(0x01);
   delay_ms(5);
}

void LCD_GotoXY(uint8_t row, uint8_t col){
    uint8_t addr;
    if(row == 0) addr = 0x80 + col;
    else         addr = 0xC0 + col;
    LCD_SendCmd(addr);
}

void LCD_Print(char *str){
    while(*str){
        LCD_SendData(*str++);
    }
}
void LCD_Print_Number(uint16_t num){
char str[10];
	int i=0;
	if(num==0){
	LCD_SendData('0');
		return;
	}
	while(num>0){
	str[i++]=(num%10)+'0';
		num/=10;
	}
	while(i--){
	LCD_SendData(str[i]);
	}
	
}

