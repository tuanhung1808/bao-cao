# Project nha kinh STM32F103C8T6 + FreeRTOS (Keil C / ARMCC)

## Cau truc project (theo mau RTOS_KeyPad_LCD)

- `USER/`: ma nguon ung dung nha kinh va driver do nhom viet.
- `MDK/Greenhouse_APP.uvprojx`: project Keil uVision.
- `MDK/Lib/CMSIS/`: CMSIS core, device header va startup support.
- `MDK/Lib/STM32F10x_StdPeriph_Driver/`: thu vien Standard Peripheral.
- `MDK/RTE/Device/STM32F103C8/`: startup va system_stm32f10x.c.
- `MDK/freertos_add/`: cac file `.c` cua FreeRTOS V9 theo project mau.
- `MDK/freertos_add/include/`: cac header FreeRTOS va `FreeRTOSConfig.h`.
- `MDK/Objects/`, `MDK/Listings/`: output do Keil tao.

## Mo va build

1. Giai nen toan bo thu muc `Greenhouse_APP` vao mot thu muc lam viec.
2. Mo `MDK/Greenhouse_APP.uvprojx` trong Keil uVision voi ARM Compiler 5.
3. Build target `Greenhouse_APP`.
4. Sau khi build thanh cong, dung `MDK/Create_BIN.bat` (tu Keil Command Prompt) de tao `MDK/Objects/greenhouse_app.bin`.
5. Firmware app duoc link tu `0x0800C800`, kich thuoc vung app `0x13000` byte (76 KiB). `SystemInit` dung `VECT_TAB_OFFSET=0xC800`.

## Pin va relay

- PA0: ADC soil moisture; PA2: ADC water-level strip module; PA3: DHT22.
- PA5=OK/MENU, PA6=UP, PA7=DOWN, PB0=BACK (active-low pull-up).
- PB6=SCL, PB7=SDA: OLED SSD1306 + BH1750 + DS3231 (I2C1).
- PA9=USART1 TX, PA10=USART1 RX: ESP32 at 115200 baud.
- PB12=relay IN1 fan, PB13=IN2 lamp, PB14=IN3 pump, PB15=IN4 spare. Relay outputs are active-low.
- PB1 buzzer; PC13 status LED.

## FreeRTOS

Kernel source files are included under `MDK/freertos_add` in the same format as `RTOS_KeyPad_LCD.zip` (FreeRTOS V9). `FreeRTOSConfig.h` is kept under `MDK/freertos_add/include`.

## Important hardware checks

- Calibrate `SOIL_RAW_DRY`, `SOIL_RAW_WET`, and `WATER_RAW_MIN` in `USER/app_config.h` for the exact sensor modules before connecting the pump.
- Current water-level strip is treated as an analog input on PA2. If your specific board only exposes digital `DO`, change wiring and driver accordingly.
- Timer schedule attempts water for 10 seconds regardless of soil humidity, but pump remains blocked if the tank is low, temperature data is invalid, or temperature is at/above `waterBlockTempC` while soil is dry (at/below `soilOnPercent`).
- The config page and metadata page must remain outside the bootloader's application erase range. See `Docs/BOOTLOADER_FLASH_PATCH.txt` before using OTA.

This archive was reorganized to match the directory and FreeRTOS-file layout of the supplied `RTOS_KeyPad_LCD` example. Keil/ARMCC was not available in the build environment, so compile and hardware testing still need to be run in Keil on the team's setup.
