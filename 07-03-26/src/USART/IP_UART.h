#ifndef __IP_UART_H
#include "stm32f10x.h"

void IP_UART_Init_CR1_PA9PA10(void);
void IP_UART_Tx(uint16_t *data);
void IP_UART_Rx(uint16_t *data);
void IP_USART_Init(USART_TypeDef *USARTx);
void int2char(int num, char *str);
void UART_SendChar(USART_TypeDef* USARTx, char _chr);
void UART_SendStr(USART_TypeDef* USARTx,char *str);


#endif
