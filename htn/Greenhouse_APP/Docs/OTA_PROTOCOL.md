# Tích hợp OTA cho firmware nhà kính

Ứng dụng này có lệnh ở USART1:
- ESP32 -> STM32: ASCII `OTA_REQ\r\n`
- STM32 -> ESP32: ASCII `OTA_ACK\r\n`
- Sau 250 ms STM32 gọi `NVIC_SystemReset()`. Bootloader cũ sẽ chờ byte `0xAA` trong 10 s.
- Sau khi nhận `OTA_ACK`, ESP32 phải đợi board reset rồi phát gói nhị phân theo giao thức hiện có: `0xAA | len_hi | len_lo | payload | sum16_hi | sum16_lo`; kết thúc `0xAA 0x00 0x00`.

Code ESP32 trong ZIP hiện tại chưa gửi `OTA_REQ` hoặc chờ `OTA_ACK`. Vì vậy, luồng hiện tại vẫn là: upload BIN -> reset STM32 -> bấm Send trong 10 giây. App chỉ bổ sung khả năng nhận lệnh để có thể tự động hóa ở bước tích hợp tiếp theo.
