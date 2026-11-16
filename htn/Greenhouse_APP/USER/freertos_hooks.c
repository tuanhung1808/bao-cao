#include "FreeRTOS.h"
#include "task.h"
#include "board.h"

/* Fault hooks: stop all load outputs if task allocation/stack check fails. */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    Board_AllRelaysOff();
    Board_SetBuzzer(1U);
    for (;;) { }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *taskName)
{
    (void)task;
    (void)taskName;
    taskDISABLE_INTERRUPTS();
    Board_AllRelaysOff();
    Board_SetBuzzer(1U);
    for (;;) { }
}
