#ifndef __BUTTON_H
#define __BUTTON_H

#include "stm32f10x.h"
#include "SYSTICK.h"

#define DEBOUNCE_DELAY 20 


typedef struct {
    uint32_t Pin;               
    uint32_t LastDebounceTime; 
    uint8_t  CurrentState;      
    uint8_t  LastRawState;      
    uint8_t  IsPressed;         
} Button_TypeDef;


extern Button_TypeDef Btn_Up;
extern Button_TypeDef Btn_Down;
extern Button_TypeDef Btn_Rev;


extern uint32_t millis(void);

void Button_Update(Button_TypeDef *btn);
uint8_t Button_Read_Event(Button_TypeDef *btn);

#endif
