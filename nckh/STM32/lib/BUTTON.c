#include "BUTTON.h"

Button_TypeDef Btn_Up   = {3, 0, 1, 1, 0}; 
Button_TypeDef Btn_Down = {4, 0, 1, 1, 0};
Button_TypeDef Btn_Rev  = {2, 0, 1, 1, 0};

void Button_Update(Button_TypeDef *btn) {
 
    uint8_t raw_state = (GPIOA->IDR & (1 << btn->Pin)) ? 1 : 0;
    
  
    if (raw_state != btn->LastRawState) {
    
        btn->LastDebounceTime = millis(); 
    }
    
   
    if ((millis() - btn->LastDebounceTime) >= DEBOUNCE_DELAY) {
        
        if (raw_state != btn->CurrentState) {
            btn->CurrentState = raw_state; 
            
          
            if (btn->CurrentState == 0) {
                btn->IsPressed = 1; 
            }
        }
    }
    
    
    btn->LastRawState = raw_state;
}

uint8_t Button_Read_Event(Button_TypeDef *btn) {
    
    if (btn->IsPressed == 1) {
        btn->IsPressed = 0; 
        return 1; 
    }
    return 0; 
}
