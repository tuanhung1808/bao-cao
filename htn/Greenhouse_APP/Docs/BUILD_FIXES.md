# Cac loi build da sua (Keil ARMCC V5.06)

## 1. `FreeRTOS.h must appear ... before task.h`
- `USER/main.c`: include `FreeRTOS.h` truoc `task.h`.

## 2. `xTaskGetSchedulerState` / `taskSCHEDULER_RUNNING` chua duoc khai bao
- `USER/app_state.c`: them `#include "task.h"`.
- `INCLUDE_xTaskGetSchedulerState` da duoc bat trong `MDK/freertos_add/include/FreeRTOSConfig.h`.

## 3. `FLASH_KEY1`, `FLASH_KEY2`, `RDP_Key` khong xac dinh trong driver cua Keil Pack
- Project co san ban StdPeriph V3.5.0 trong `MDK/Lib/STM32F10x_StdPeriph_Driver/src`.
- Da bo cac RTE component StdPeriph Drivers tu `Greenhouse_APP.uvprojx`; nhung component nay lam Keil them driver khac tu STM32F1xx_DFP va trung voi driver da co trong project.
- Project tiep tuc dung mot bo StdPeriph V3.5.0 duy nhat trong `MDK/Lib`.

## 4. `VECT_TAB_OFFSET` bi dinh nghia lai
- `system_stm32f10x.c` gio chi dat gia tri mac dinh 0 neu chua co `VECT_TAB_OFFSET` tu project.
- Target define `VECT_TAB_OFFSET=0xC800` va `main.c` dat `SCB->VTOR = 0x0800C800`.

## 5. Canh bao enum khi doc button
- `Board_ButtonPressed()` dung bien `uint8_t` de luu gia tri doc GPIO, tranh canh bao chuyen doi enum.

## Cach build lai
1. Mo `MDK/Greenhouse_APP.uvprojx`.
2. Chon `Project > Clean Targets`.
3. Chon `Project > Rebuild all target files`.
4. Neu Keil van hien source cua STM32F1xx_DFP cu trong Build Output, dong project va mo lai file `.uvprojx` nay; kiem tra tab Manage Run-Time Environment khong tu them lai StdPeriph Drivers vi project da dung driver local.

Luu y: Project da duoc kiem tra XML va duong dan file. Moi truong tao ZIP khong co Keil ARMCC 5.06 de chay build thuc te, nen can Rebuild tren may Keil de xac nhan ket qua cuoi cung.
