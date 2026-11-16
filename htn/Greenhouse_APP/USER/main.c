#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "app_tasks.h"

int main(void)
{
    /* The application is linked at 0x0800C800. This also protects interrupt
     * vector lookup when the bootloader jumps to the application. VECT_TAB_OFFSET
     * is defined in the Keil project as 0xC800 for SystemInit(). */
    SCB->VTOR = 0x0800C800UL;
    SystemCoreClockUpdate();

    Board_Init();
    Board_ADC_Init();
    Board_I2C1_Init();
    Board_UART1_Init(115200UL);

    App_CreateTasks();
    vTaskStartScheduler();

    /* Reaching here means FreeRTOS could not allocate its idle task/heap. */
    Board_AllRelaysOff();
    for (;;) { Board_SetStatusLED(1U); }
}
