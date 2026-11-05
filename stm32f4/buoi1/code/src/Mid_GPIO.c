#include "Mid GPIO.h"

void Mid GPIO OutPut_Configuration (GPIO_TypeDef GPIOx, uint32_t pinSelect)

if (GPIOx != GPIOA || GPIOX != GPIOB | | GPIOX != GPIOC | | GPIOX != GPIOD)
{
while(1) ;
}
if(pinSelect >= 16)
{
while(1) ;
}
Ip_GPIO_Output_Configuration (&GPIOx, pinSelect);


}