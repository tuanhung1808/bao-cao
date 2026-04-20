#include "IP_UART.h"

void IP_UART_Init_CR1_PA9PA10(){
	    /* b?t clock */
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN |
                    RCC_APB2ENR_IOPAEN |
                    RCC_APB2ENR_USART1EN;

    /* PA9 TX */
    GPIOA->CRH &= ~(0xF << 4);
    GPIOA->CRH |=  (0xB << 4);

    /* PA10 RX */
    GPIOA->CRH &= ~(0xF << 8);	
    GPIOA->CRH |=  (0x4 << 8);

    /* baudrate 9600 @72MHz */
    USART1->BRR = 0x1D4C;

    /* enable TX + RX */
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;

    /* enable USART */
    USART1->CR1 |= USART_CR1_UE;
}

void IP_UART_Tx(uint16_t *data){
	for(uint8_t i=0;i<sizeof(data);i++){
		USART1->DR = *(data+i);
		while((USART1->SR & (1<<6)) ==0); //doi den khi truyen xong
	}
	
}

void IP_UART_Rx(uint16_t *data){
		for(uint8_t i=0;i<sizeof(data);i++){
		 *(data + i) =USART1->DR;
		while((USART1->SR & (1<<5)) ==0); //doi den khi nhan tin hieu
		//while((USART1->SR & 0x20) == 0x00);	
	}
}

void IP_USART_Init(USART_TypeDef *USARTx){

	if(USARTx == USART1){
IP_UART_Init_CR1_PA9PA10();
	}
	else if(USARTx == USART2){
		RCC->APB2ENR |= 1;
  RCC->APB1ENR |= 0x20000;
		GPIOA->CRL |= 0x0B00; //PA2
		GPIOA->CRL |= 0x8000; //PA3
		USART2->BRR = 0xEA6; //baude rate
		USART2->CR1 |=8; //TE
		USART2->CR1 |=4; //RE
		USART2->CR1 |= 0x2000; //UE
	}
	else if(USARTx == USART3){
				RCC->APB2ENR |= 1;
  RCC->APB1ENR |= 0x40000;
		GPIOA->CRH |= 0x0B00; //PB10
		GPIOA->CRH |= 0x8000; //PB11
		USART3->BRR = 0x0341; //baude rate
//		USART3->BRR = 0x0139;
		
		USART3->CR1 |=8; //TE
		USART3->CR1 |=4; //RE
		USART3->CR1 |= 0x2000; //UE
	}
}

void int2char(int num, char *str)
{
	int i = 0;
	int j = 0;
	char temp;

	// Tru?ng h?p s? 0
	if(num == 0)
	{
		str[i++] = '0';
		str[i] = '\0';
		return;
	}

	// Tách t?ng ch? s? (ngu?c)
	while(num > 0)
	{
		str[i++] = (num % 10) + '0';
		num /= 10;
	}

	str[i] = '\0';

	// Ð?o chu?i l?i cho dúng th? t?
	j = 0;
	i = i - 1;
	while(j < i)
	{
		temp = str[j];
		str[j] = str[i];
		str[i] = temp;
		j++;
		i--;
	}
}
void UART_SendChar(USART_TypeDef* USARTx, char _chr)
{
    while (!(USARTx->SR & (1 << 7)));   // TXE = 1 bat  // SR = thanh ghi SR kiem tra USART có trong hay khong 

    USARTx->DR = _chr;  // Dung de ghi doc du lieu ra , vao

}


void UART_SendStr(USART_TypeDef* USARTx,char *str)
{
    while (*str != '\0')
    {
        UART_SendChar(USARTx,*str++);
    }
}


