/*
 * main.c - GMD / Gowin_PicoRV32
 * SOC DASHBOARD: Tích hợp I2C (DS3231 + BH1750), SPI TFT và UART
 * BẢN FIX: Chống treo BH1750 & Fix lỗi sprintf "u lx"
 */
#include <stdint.h>
#include <stdio.h>
#include "picorv32.h"
#include "wbuart.h"
#include "wbi2c.h"
#include "wbspi.h"
#include "wbgpio.h"

uint32_t SystemCoreClock = 27000000;
volatile uint32_t irq00_flag = 0; volatile uint32_t irq13_flag = 0;
volatile uint32_t irq20_flag = 0; volatile uint32_t irq21_flag = 0;

// --- ĐỊNH NGHĨA PHẦN CỨNG ---
#define BH1750_ADDR  0x23
#define DS3231_ADDR  0x68
#define I2C_TIMEOUT  1000000UL

#define GPIOP PICO_WBGPIO
#define SPIP  PICO_WBSPI_MASTER
#define DC  (1u << 0)
#define RES (1u << 1)
#define X_OFF 40
#define Y_OFF 52

// --- FONT 8x8 ---
const uint8_t font[95][8] = {
    {0,0,0,0,0,0,0,0},{24,60,60,24,24,0,24,0},{102,102,36,0,0,0,0,0},{108,108,254,108,254,108,108,0},
    {24,62,96,60,6,124,24,0},{0,198,204,24,48,102,198,0},{56,108,56,118,220,204,118,0},{24,24,48,0,0,0,0,0},
    {12,24,48,48,48,24,12,0},{48,24,12,12,12,24,48,0},{0,102,60,255,60,102,0,0},{0,24,24,126,24,24,0,0},
    {0,0,0,0,0,24,24,48},{0,0,0,126,0,0,0,0},{0,0,0,0,0,24,24,0},{6,12,24,48,96,192,128,0},
    {60,102,110,118,102,102,60,0},{24,56,24,24,24,24,126,0},{60,102,6,28,48,96,126,0},{60,102,6,28,6,102,60,0},
    {28,60,108,204,254,12,12,0},{126,96,124,6,6,102,60,0},{60,102,96,124,102,102,60,0},{126,6,12,24,48,48,48,0},
    {60,102,102,60,102,102,60,0},{60,102,102,62,6,102,60,0},{0,24,24,0,0,24,24,0},{0,24,24,0,0,24,24,48},
    {12,24,48,96,48,24,12,0},{0,0,126,0,126,0,0,0},{48,24,12,6,12,24,48,0},{60,102,6,12,24,0,24,0},
    {60,102,110,110,96,102,60,0},{60,102,102,126,102,102,102,0},{124,102,102,124,102,102,124,0},{60,102,96,96,96,102,60,0},
    {120,108,102,102,102,108,120,0},{126,96,96,124,96,96,126,0},{126,96,96,124,96,96,96,0},{60,102,96,110,102,102,60,0},
    {102,102,102,126,102,102,102,0},{60,24,24,24,24,24,60,0},{6,6,6,6,6,102,60,0},{102,108,120,112,120,108,102,0},
    {96,96,96,96,96,96,126,0},{99,119,127,107,99,99,99,0},{102,118,126,126,110,102,102,0},{60,102,102,102,102,102,60,0},
    {124,102,102,124,96,96,96,0},{60,102,102,102,106,108,54,0},{124,102,102,124,108,102,102,0},{60,102,96,60,6,102,60,0},
    {126,24,24,24,24,24,24,0},{102,102,102,102,102,102,60,0},{102,102,102,102,102,60,24,0},{99,99,99,107,127,119,99,0},
    {102,102,60,24,60,102,102,0},{102,102,102,60,24,24,24,0},{126,6,12,24,48,96,126,0},{60,48,48,48,48,48,60,0},
    {128,192,96,48,24,12,6,0},{60,12,12,12,12,12,60,0},{24,60,102,0,0,0,0,0},{0,0,0,0,0,0,255,0},
    {48,24,12,0,0,0,0,0},{0,0,60,6,62,102,62,0},{96,96,124,102,102,102,124,0},{0,0,60,96,96,96,60,0},
    {6,6,62,102,102,102,62,0},{0,0,60,102,126,96,60,0},{28,48,48,124,48,48,48,0},{0,0,62,102,102,62,6,60},
    {96,96,124,102,102,102,102,0},{24,0,56,24,24,24,60,0},{6,0,6,6,6,102,60,0},{96,96,102,108,120,108,102,0},
    {56,24,24,24,24,24,60,0},{0,0,118,127,107,107,107,0},{0,0,124,102,102,102,102,0},{0,0,60,102,102,102,60,0},
    {0,0,124,102,102,124,96,96},{0,0,62,102,102,62,6,6},{0,0,124,102,96,96,96,0},{0,0,60,96,60,6,60,0},
    {48,48,124,48,48,48,28,0},{0,0,102,102,102,102,62,0},{0,0,102,102,102,60,24,0},{0,0,99,107,127,119,99,0},
    {0,0,102,60,24,60,102,0},{0,0,102,102,102,62,6,60},{0,0,126,12,24,48,126,0},{14,24,24,112,24,24,14,0},
    {24,24,24,0,24,24,24,0},{112,24,24,14,24,24,112,0},{58,110,0,0,0,0,0,0}
};

// ============================================================================
// HÀM TIỆN ÍCH
// ============================================================================
static uint32_t get_cpu_cycles(void) {
    uint32_t cycles; __asm__ volatile ("rdcycle %0" : "=r" (cycles)); return cycles;
}
static void delay_ms(uint32_t ms) {
    uint32_t start = get_cpu_cycles(), wait = ms * (SystemCoreClock / 1000);
    while ((get_cpu_cycles() - start) < wait);
}
static void uart_puts(const char *str) { while (*str) wbuart_putc(*str++); }
static uint8_t bcd_to_dec(uint8_t val) { return (uint8_t)(((val >> 4) * 10) + (val & 0x0F)); }

// ============================================================================
// DRIVER I2C VÀ CẢM BIẾN
// ============================================================================
static int i2c_wait_tip(void) {
    uint32_t to = I2C_TIMEOUT; while (PICO_WBI2C->SR & I2C_SR_TIP) { if (to-- == 0) return 0; } return 1;
}
static int i2c_wait_idle(void) {
    uint32_t to = I2C_TIMEOUT; while (PICO_WBI2C->SR & I2C_SR_BUSY) { if (to-- == 0) return 0; } return 1;
}
static void i2c_stop(void) { PICO_WBI2C->CR = I2C_CMD_STO; i2c_wait_tip(); i2c_wait_idle(); }

static int i2c_write_byte(uint8_t data, int start, int stop) {
    uint32_t cmd = I2C_CMD_WR;
    PICO_WBI2C->TXR = data;
    if (start) cmd |= I2C_CMD_STA;
    if (stop)  cmd |= I2C_CMD_STO;
    PICO_WBI2C->CR = cmd;
    if (!i2c_wait_tip()) return -1;
    if (PICO_WBI2C->SR & I2C_SR_RXACK) return 0;
    return 1;
}

// Hàm đánh thức BH1750 (Đã fix lỗi bị ngủ quên)
static void bh1750_wake_up(void) {
    // 1. Gửi Power On trước để đảm bảo cảm biến thức dậy
    if (i2c_write_byte(BH1750_ADDR << 1, 1, 0) == 1) {
        i2c_write_byte(0x01, 0, 1);
    } else { i2c_stop(); return; }

    delay_ms(5); // Chờ 5ms

    // 2. Gửi lệnh Đo 1 lần (One-Time H-Res)
    if (i2c_write_byte(BH1750_ADDR << 1, 1, 0) == 1) {
        i2c_write_byte(0x20, 0, 1);
    } else { i2c_stop(); }
}

static int bh1750_get_lux(uint32_t *lux) {
    if (i2c_write_byte((BH1750_ADDR << 1) | 1, 1, 0) != 1) { i2c_stop(); return 0; }
    PICO_WBI2C->CR = I2C_CMD_RD;
    if (!i2c_wait_tip()) { i2c_stop(); return 0; }
    uint8_t msb = (uint8_t)(PICO_WBI2C->RXR & 0xFF);
    PICO_WBI2C->CR = I2C_CMD_RD | I2C_CMD_ACK | I2C_CMD_STO;
    if (!i2c_wait_tip()) { i2c_stop(); return 0; }
    uint8_t lsb = (uint8_t)(PICO_WBI2C->RXR & 0xFF);
    if (!i2c_wait_idle()) return 0;

    *lux = ((uint32_t)(((uint16_t)msb << 8) | lsb) * 10 + 6) / 120;
    return 1;
}

static int ds3231_get_time(uint8_t *h, uint8_t *m, uint8_t *s) {
    if (i2c_write_byte((DS3231_ADDR << 1), 1, 0) != 1)     { i2c_stop(); return 0; }
    if (i2c_write_byte(0x00, 0, 0) != 1)                   { i2c_stop(); return 0; }
    if (i2c_write_byte((DS3231_ADDR << 1) | 1, 1, 0) != 1) { i2c_stop(); return 0; }
    for (int i = 0; i < 3; i++) {
        PICO_WBI2C->CR = (i == 2) ? (I2C_CMD_RD | I2C_CMD_ACK | I2C_CMD_STO) : I2C_CMD_RD;
        if (!i2c_wait_tip()) { i2c_stop(); return 0; }
        uint8_t b = (uint8_t)(PICO_WBI2C->RXR & 0xFF);
        if (i == 0) *s = bcd_to_dec(b & 0x7F);
        if (i == 1) *m = bcd_to_dec(b & 0x7F);
        if (i == 2) *h = (b & 0x40) ? bcd_to_dec(b & 0x1F) : bcd_to_dec(b & 0x3F);
    }
    if (!i2c_wait_idle()) return 0;
    return 1;
}

// ============================================================================
// DRIVER SPI TFT
// ============================================================================
static uint32_t gpio_out = 0;
static void gpio_set(uint32_t m) { gpio_out |=  m; GPIO_WriteData(GPIOP, gpio_out); }
static void gpio_clr(uint32_t m) { gpio_out &= ~m; GPIO_WriteData(GPIOP, gpio_out); }
static void tft_cs_low(void)  { SPIP->SSMASK = 1; }
static void tft_cs_high(void) { SPIP->SSMASK = 0; }
static void spi_send(uint8_t d) { wbspi_master_txdata(SPIP, d); }

static void tft_cmd(uint8_t c) {
    tft_cs_high();
    for(volatile int w=0; w<10; w++) __asm__("nop");
    gpio_clr(DC); tft_cs_low(); spi_send(c);
}
static void tft_dat(uint8_t d) { gpio_set(DC); spi_send(d); }

static void window(int x0, int y0, int x1, int y1) {
    x0 += X_OFF; x1 += X_OFF; y0 += Y_OFF; y1 += Y_OFF;
    tft_cmd(0x2A); tft_dat(x0>>8); tft_dat(x0); tft_dat(x1>>8); tft_dat(x1);
    tft_cmd(0x2B); tft_dat(y0>>8); tft_dat(y0); tft_dat(y1>>8); tft_dat(y1);
    tft_cmd(0x2C); gpio_set(DC);
}

static void tft_init(void) {
    GPIO_Init(GPIOP); GPIO_WriteData(GPIOP, 0); GPIO_SetDir(GPIOP, DC | RES);
    tft_cs_high();
    gpio_set(RES); delay_ms(50); gpio_clr(RES); delay_ms(150); gpio_set(RES); delay_ms(150);
    tft_cmd(0x11); delay_ms(150);
    tft_cmd(0x36); tft_dat(0x70); // Landscape
    tft_cmd(0x3A); tft_dat(0x05);
    tft_cmd(0x21); tft_cmd(0x13); tft_cmd(0x29); delay_ms(50);
}

static void clear_screen(void) {
    window(0, 0, 239, 134);
    tft_cs_low();
    for (uint32_t i = 0; i < 240UL * 135UL * 2UL; i++) spi_send(0x00);
    tft_cs_high();
}

static void in_chu(int x, int y, const char *s, int scale) {
    while (*s) {
        char c = *s++; if (c < 32 || c > 126) c = '?';
        window(x, y, x + (8 * scale) - 1, y + (8 * scale) - 1);
        tft_cs_low();
        for (int r = 0; r < 8; r++) {
            for (int sy = 0; sy < scale; sy++) {
                for (int b = 7; b >= 0; b--) {
                    uint8_t v = (font[c - 32][r] & (1 << b)) ? 0xFF : 0x00;
                    for (int sx = 0; sx < scale; sx++) { spi_send(v); spi_send(v); }
                }
            }
        }
        tft_cs_high(); x += (8 * scale);
    }
}

// ============================================================================
// HÀM MAIN - CHẠY TOÀN HỆ THỐNG
// ============================================================================
int main(void) {
    // 1. Khởi tạo Giao tiếp
    wbuart_init(115200);
    delay_ms(50);
    uart_puts("\r\n--- TANG NANO 9K SOC BOOTING ---\r\n");

    PICO_WBI2C->CHANNEL = I2C_CHANNEL_0;
    PICO_WBI2C->PRER = 53;
    PICO_WBI2C->CTR = I2C_CTR_EN;

    tft_init();
    clear_screen();

    // 2. Vẽ UI tĩnh (Chỉ vẽ 1 lần)
    in_chu(15,  15, "SOC DASHBOARD", 2);
    in_chu(15,  60, "TIME:", 2);
    in_chu(15, 100, "LUX :", 2);

    // Biến cập nhật
    uint8_t gio, phut, giay;
    uint32_t lux;
    char tft_buf[32];
    char uart_buf[64];

    uart_puts("--- SYSTEM READY ---\r\n");

    // 3. Vòng lặp chính
    while (1) {
        // Đánh thức BH1750
        bh1750_wake_up();

        // Chờ cảm biến phân tích ánh sáng
        delay_ms(150);

        // Lấy dữ liệu
        int rtc_ok = ds3231_get_time(&gio, &phut, &giay);
        int lux_ok = bh1750_get_lux(&lux);

        // --- CẬP NHẬT TFT ---
        if (rtc_ok) {
            sprintf(tft_buf, "%02d:%02d:%02d", gio, phut, giay);
            in_chu(95, 60, tft_buf, 2);
        } else {
            in_chu(95, 60, "ERR: RTC", 2);
        }

        if (lux_ok) {
            // FIX LỖI U LX: Ép kiểu sang (int) và dùng %d thay vì %lu
            // Khoảng trắng ở đuôi để xóa các số thừa của vòng lặp trước
            sprintf(tft_buf, "%d lx   ", (int)lux);
            in_chu(95, 100, tft_buf, 2);
        } else {
            in_chu(95, 100, "ERR: LUX", 2);
        }

        // --- GỬI LOG RA UART ---
        if (rtc_ok && lux_ok) {
            // In 1 dòng duy nhất thật đẹp ra Terminal
            sprintf(uart_buf, "[DATA] Time: %02d:%02d:%02d | Light: %d lx\r\n", gio, phut, giay, (int)lux);
            uart_puts(uart_buf);
        } else {
            uart_puts("[WARN] Sensor Error!\r\n");
        }

        // Delay 850ms để tròn chu kỳ 1 giây
        delay_ms(850);
    }

    return 0;
}
