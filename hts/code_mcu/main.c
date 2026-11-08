/*
 * main.c - Gowin PicoRV32 / Tang Nano 9K
 * TFT ST7789 + DS3231 + BH1750 + 4 GPIO buttons
 * BẢN CHUẨN KẾT HỢP: UI TUYỆT ĐẸP + KIẾN TRÚC MULTI-TASKING NON-BLOCKING
 * Đã FIX lỗi biên dịch (Undeclared identifiers: DC, RES, SCR_W, SCR_H)
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "picorv32.h"
#include "wbuart.h"
#include "wbi2c.h"
#include "wbspi.h"
#include "wbgpio.h"

uint32_t SystemCoreClock = 27000000;

volatile uint32_t irq00_flag = 0; volatile uint32_t irq13_flag = 0;
volatile uint32_t irq20_flag = 0; avolatile uint32_t irq21_flag = 0;

// --- Cấu hình Phần cứng ---
#define BH1750_ADDR       0x23
#define DS3231_ADDR       0x68
#define I2C_TIMEOUT       1000000UL
#define GPIOP             PICO_WBGPIO
#define SPIP              PICO_WBSPI_MASTER

// ĐÃ FIX: Đồng bộ đúng tên biến GPIO và Kích thước màn hình
#define DC                (1u << 0)
#define RES               (1u << 1)
#define SCR_W             240
#define SCR_H             135
#define X_OFF             40
#define Y_OFF             52

// --- Hệ Màu Sắc RGB565 ---
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_CYAN        0x07FF
#define COLOR_TEAL        0x1D6A
#define COLOR_GRAY        0x8410
#define COLOR_YELLOW      0xFFE0
#define COLOR_DARKGRAY    0x4208

#define DEFAULT_YEAR   2026
#define DEFAULT_MONTH  11

#define RTC_POLL_MS    200
#define BH_PERIOD_MS   1000
#define BH_MEAS_MS     180
#define BTN_SCAN_MS    10

typedef struct {
    int      ok;
    uint8_t  sec, min, hour, date, month;
    uint16_t year;
} rtc_t;

typedef enum { SCR_MAIN, SCR_MENU, SCR_CAL, SCR_STOPWATCH } screen_t;

#define BTN_MENU 0x01
#define BTN_UP   0x02
#define BTN_DOWN 0x04
#define BTN_BACK 0x08

// ---------- FONT 8x8 ----------
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
// TIEN ICH
// ============================================================================
static uint32_t get_cpu_cycles(void) {
    uint32_t cycles; __asm__ volatile ("rdcycle %0" : "=r" (cycles)); return cycles;
}
static uint32_t ms_to_cycles(uint32_t ms) { return ms * (SystemCoreClock / 1000); }
static void delay_ms(uint32_t ms) {
    uint32_t start = get_cpu_cycles(), wait = ms_to_cycles(ms);
    while ((get_cpu_cycles() - start) < wait);
}
static uint8_t bcd_to_dec(uint8_t value) { return (uint8_t)(((value >> 4) * 10) + (value & 0x0F)); }

static char *put_u(char *p, uint32_t v, int min_digits) {
    char t[10]; int n = 0;
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v != 0 && n < 10);
    while (n < min_digits && n < 10) t[n++] = '0';
    while (n > 0) *p++ = t[--n];
    return p;
}
static char *put_s(char *p, const char *s) { while (*s) *p++ = *s++; return p; }
static int str_len(const char *s) { int n = 0; while (s[n]) n++; return n; }
static void pad_str(char *dst, const char *src, int width) {
    int i = 0; while (src[i] && i < width) { dst[i] = src[i]; i++; }
    while (i < width) dst[i++] = ' '; dst[i] = 0;
}
static void center_str(char *dst, const char *src, int width) {
    int len = str_len(src), left = (width - len) / 2, i = 0;
    if (left < 0) left = 0;
    for (; i < left && i < width; i++) dst[i] = ' ';
    for (int j = 0; src[j] && i < width; j++) dst[i++] = src[j];
    while (i < width) dst[i++] = ' '; dst[i] = 0;
}
static void fmt_right(char *dst, uint32_t v, int width) {
    char t[12]; char *e = put_u(t, v, 1);
    int len = (int)(e - t), pad = width - len; char *p = dst;
    if (pad < 0) pad = 0;
    while (pad-- > 0) *p++ = ' ';
    for (int i = 0; i < len; i++) *p++ = t[i];
    *p = 0;
}

// ---------- Lich ----------
static int is_leap(int y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); }
static int days_in_month(int y, int m) {
    static const uint8_t d[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && is_leap(y)) return 29; return d[m - 1];
}
static int day_of_week(int y, int m, int d) {
    static const int t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y--; return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

static const char *const month_name[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
static const char *const wday_name[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *const wday_short[7] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

// ============================================================================
// GPIO + NUT BAM
// ============================================================================
static uint32_t gpio_out = 0;
static void gpio_set(uint32_t m) { gpio_out |=  m; GPIO_WriteData(GPIOP, gpio_out); }
static void gpio_clr(uint32_t m) { gpio_out &= ~m; GPIO_WriteData(GPIOP, gpio_out); }

static uint8_t read_buttons_pressed(void) {
    uint32_t in = GPIO_ReadData(GPIOP); return (uint8_t)((~in >> 2) & 0x0F);
}

static uint8_t buttons_poll(uint32_t now) {
    static uint32_t last = 0; static uint8_t prev = 0, state = 0; uint8_t ev = 0;
    if ((now - last) < ms_to_cycles(BTN_SCAN_MS)) return 0;
    last = now;
    uint8_t raw = read_buttons_pressed();
    if (raw == prev) { ev = raw & (uint8_t)(~state); state = raw; }
    prev = raw; return ev;
}

// ============================================================================
// I2C (DS3231 + BH1750)
// ============================================================================
static int i2c_wait_tip(void) { uint32_t t = I2C_TIMEOUT; while (PICO_WBI2C->SR & I2C_SR_TIP) { if (t == 0) return 0; t--; } return 1; }
static int i2c_wait_idle(void) { uint32_t t = I2C_TIMEOUT; while (PICO_WBI2C->SR & I2C_SR_BUSY) { if (t == 0) return 0; t--; } return 1; }
static void i2c_stop(void) { PICO_WBI2C->CR = I2C_CMD_STO; (void)i2c_wait_tip(); (void)i2c_wait_idle(); }
static int i2c_write_byte(uint8_t data, int start, int stop) {
    uint32_t cmd = I2C_CMD_WR; PICO_WBI2C->TXR = data;
    if (start) cmd |= I2C_CMD_STA; if (stop) cmd |= I2C_CMD_STO;
    PICO_WBI2C->CR = cmd;
    if (!i2c_wait_tip()) return -1;
    if (PICO_WBI2C->SR & I2C_SR_RXACK) return 0; return 1;
}

static void i2c_init(void) {
    PICO_WBI2C->CHANNEL = I2C_CHANNEL_0; PICO_WBI2C->PRER = 53; PICO_WBI2C->CTR = I2C_CTR_EN; delay_ms(10);
}

static void bh1750_start(void) {
    if (i2c_write_byte(BH1750_ADDR << 1, 1, 0) == 1) { i2c_write_byte(0x01, 0, 1); } else { i2c_stop(); return; }
    delay_ms(5);
    if (i2c_write_byte(BH1750_ADDR << 1, 1, 0) == 1) { i2c_write_byte(0x20, 0, 1); } else { i2c_stop(); }
}

static int bh1750_read_lux(uint32_t *lux) {
    if (i2c_write_byte((BH1750_ADDR << 1) | 1, 1, 0) != 1) { i2c_stop(); return 0; }
    PICO_WBI2C->CR = I2C_CMD_RD; if (!i2c_wait_tip()) { i2c_stop(); return 0; }
    uint8_t msb = (uint8_t)(PICO_WBI2C->RXR & 0xFF);
    PICO_WBI2C->CR = I2C_CMD_RD | I2C_CMD_ACK | I2C_CMD_STO; if (!i2c_wait_tip()) { i2c_stop(); return 0; }
    uint8_t lsb = (uint8_t)(PICO_WBI2C->RXR & 0xFF); if (!i2c_wait_idle()) return 0;
    uint32_t raw = ((uint32_t)msb << 8) | lsb; *lux = (raw * 10 + 6) / 12; return 1;
}

static int ds3231_read_regs(uint8_t reg, uint8_t *buffer, uint32_t length) {
    if (buffer == 0 || length == 0) return 0;
    if (i2c_write_byte((DS3231_ADDR << 1), 1, 0) != 1) { i2c_stop(); return -1; }
    if (i2c_write_byte(reg, 0, 0) != 1) { i2c_stop(); return -1; }
    if (i2c_write_byte((DS3231_ADDR << 1) | 1, 1, 0) != 1) { i2c_stop(); return -1; }
    for (uint32_t i = 0; i < length; i++) {
        PICO_WBI2C->CR = (i == length - 1) ? (I2C_CMD_RD | I2C_CMD_ACK | I2C_CMD_STO) : I2C_CMD_RD;
        if (!i2c_wait_tip()) { i2c_stop(); return -1; }
        buffer[i] = (uint8_t)(PICO_WBI2C->RXR & 0xFF);
    }
    if (!i2c_wait_idle()) return -1; return 1;
}

static void rtc_read(rtc_t *r) {
    uint8_t b[7]; r->ok = (ds3231_read_regs(0x00, b, 7) == 1); if (!r->ok) return;
    r->sec   = bcd_to_dec(b[0] & 0x7F); r->min   = bcd_to_dec(b[1] & 0x7F);
    r->hour  = (b[2] & 0x40) ? bcd_to_dec(b[2] & 0x1F) : bcd_to_dec(b[2] & 0x3F);
    r->date  = bcd_to_dec(b[4] & 0x3F);
    r->month = bcd_to_dec(b[5] & 0x1F);
    r->year  = (uint16_t)(2000 + bcd_to_dec(b[6]) + ((b[5] & 0x80) ? 100 : 0));

    if (r->month < 1 || r->month > 12 || r->date < 1 || r->date > 31 ||
        r->hour > 23 || r->min > 59 || r->sec > 59) r->ok = 0;
}

// ============================================================================
// TFT ST7789 VÀ CÁC HÀM VẼ ĐỒ HỌA CÓ MÀU (COLORFUL)
// ============================================================================
static void tft_cs_low(void)  { SPIP->SSMASK = 1; }
static void tft_cs_high(void) { SPIP->SSMASK = 0; }

static void spi_send(uint8_t d) { wbspi_master_txdata(SPIP, d); }
static void tft_cmd(uint8_t c) {
    tft_cs_high(); for (volatile int w = 0; w < 10; w++) { __asm__("nop"); }
    gpio_clr(DC); tft_cs_low(); spi_send(c);
}
static void tft_dat(uint8_t d) { gpio_set(DC); spi_send(d); }
static void window(int x0, int y0, int x1, int y1) {
    x0 += X_OFF; x1 += X_OFF; y0 += Y_OFF; y1 += Y_OFF;
    tft_cmd(0x2A); tft_dat(x0 >> 8); tft_dat(x0); tft_dat(x1 >> 8); tft_dat(x1);
    tft_cmd(0x2B); tft_dat(y0 >> 8); tft_dat(y0); tft_dat(y1 >> 8); tft_dat(y1);
    tft_cmd(0x2C); gpio_set(DC);
}
static void tft_init(void) {
    GPIO_Init(GPIOP); gpio_out = 0; GPIO_WriteData(GPIOP, 0); GPIO_SetDir(GPIOP, DC | RES);
    tft_cs_high();
    gpio_set(RES); delay_ms(50); gpio_clr(RES); delay_ms(150); gpio_set(RES); delay_ms(150);
    tft_cmd(0x11); delay_ms(150); tft_cmd(0x36); tft_dat(0x70);
    tft_cmd(0x3A); tft_dat(0x05); tft_cmd(0x21); tft_cmd(0x13); tft_cmd(0x29); delay_ms(50);
}
static void send_color(uint16_t c) { spi_send((uint8_t)(c >> 8)); spi_send((uint8_t)(c & 0xFF)); }
static void fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    window(x, y, x + w - 1, y + h - 1);
    tft_cs_low();
    for (uint32_t i = 0; i < (uint32_t)w * (uint32_t)h; i++) send_color(color);
    tft_cs_high();
}
static void clear_screen(void) { fill_rect(0, 0, SCR_W, SCR_H, COLOR_BLACK); }

static void tft_fill_circle(int cx, int cy, int r, uint16_t color) {
    for (int dy = -r; dy <= r; ++dy) {
        int dx = 0; while ((dx + 1) * (dx + 1) + dy * dy <= r * r) ++dx;
        fill_rect(cx - dx, cy + dy, 2 * dx + 1, 1, color);
    }
}

static void draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, int scale) {
    if (c < 32 || c > 126) c = '?';
    window(x, y, x + 8 * scale - 1, y + 8 * scale - 1);
    tft_cs_low();
    for (int row = 0; row < 8; row++) {
        for (int sy = 0; sy < scale; sy++) {
            for (int bit = 7; bit >= 0; bit--) {
                uint16_t col = (font[c - 32][row] & (1 << bit)) ? fg : bg;
                for (int sx = 0; sx < scale; sx++) send_color(col);
            }
        }
    }
    tft_cs_high();
}

static void draw_text(int x, int y, const char *s, uint16_t fg, uint16_t bg, int scale) {
    while (*s && x + 8 * scale <= SCR_W) { draw_char(x, y, *s++, fg, bg, scale); x += 8 * scale; }
}

// ============================================================================
// TRANG THAI UNG DUNG
// ============================================================================
static screen_t screen = SCR_MAIN;
static int menu_sel = 0;
#define MENU_COUNT 2

static rtc_t    rtc;
static int      lux_ok = 0;
static uint32_t lux = 0;
static int      main_dirty = 1;
static uint32_t last_draw_sec = 0xFFFFFFFF; // Luu giay de tranh ve lai man hinh HOME lien tuc

static uint32_t rtc_last = 0, bh_last = 0;
static int      bh_state = 0;

static int      cal_year = DEFAULT_YEAR, cal_month = DEFAULT_MONTH;
static uint8_t  cal_last[42];

static int      sw_running = 0;
static uint32_t sw_sec = 0, sw_acc = 0, sw_last = 0;
static uint32_t sw_last_draw = 0xFFFFFFFF;

// Chữ nhỏ dưới cùng
static void legend(const char *a, const char *b, const char *c, const char *d) {
    const char *s[4]; s[0] = a; s[1] = b; s[2] = c; s[3] = d;
    for (int i = 0; i < 4; i++) {
        int len = str_len(s[i]);
        if (len > 0) draw_text(i * 60 + (60 - len * 8) / 2, 124, s[i], COLOR_GRAY, COLOR_BLACK, 1);
    }
}

// ---------- MAIN: Giao diện màu ----------
static void draw_main(void) {
    char buf[32], *p;
    clear_screen();
    draw_text(8, 8, "HOME", COLOR_CYAN, COLOR_BLACK, 2);
    draw_text(8, 40, "TIME", COLOR_GRAY, COLOR_BLACK, 1);

    if (rtc.ok) {
        // Vẽ Giờ
        p = put_u(buf, rtc.hour, 2); *p++ = ':';
        p = put_u(p, rtc.min, 2);    *p++ = ':';
        p = put_u(p, rtc.sec, 2);    *p = 0;
        draw_text(22, 54, buf, COLOR_WHITE, COLOR_BLACK, 3);

        // Vẽ Ngày
        p = put_u(buf, rtc.date, 2); *p++ = '/';
        p = put_u(p, rtc.month, 2);  *p++ = '/';
        p = put_u(p, rtc.year, 4);   *p++ = ' '; *p++ = ' ';
        p = put_s(p, wday_name[day_of_week(rtc.year, rtc.month, rtc.date)]); *p = 0;
        draw_text(30, 91, buf, COLOR_WHITE, COLOR_BLACK, 1);
    } else {
        draw_text(22, 54, "RTC ERROR", COLOR_YELLOW, COLOR_BLACK, 2);
    }

    fill_rect(8, 111, 224, 1, COLOR_DARKGRAY);
    draw_text(10, 119, "LIGHT", COLOR_GRAY, COLOR_BLACK, 1);
    if (lux_ok) {
        p = put_u(buf, lux, 1); p = put_s(p, " lx"); *p = 0;
        draw_text(66, 119, buf, COLOR_CYAN, COLOR_BLACK, 1);
    } else {
        draw_text(66, 119, "ERR", COLOR_YELLOW, COLOR_BLACK, 1);
    }
}

// Cập nhật nhanh Lux ở HOME
static void update_lux_only(void) {
    char buf[16], *p;
    fill_rect(64, 116, 168, 16, COLOR_BLACK);
    if (lux_ok) {
        p = put_u(buf, lux, 1); p = put_s(p, " lx"); *p = 0;
        draw_text(66, 119, buf, COLOR_CYAN, COLOR_BLACK, 1);
    }
}

// ---------- MENU ----------
static void draw_menu(void) {
    clear_screen();
    draw_text(88, 12, "MENU", COLOR_CYAN, COLOR_BLACK, 2);

    fill_rect(38, 48, 164, 25, menu_sel == 0 ? COLOR_TEAL : COLOR_BLACK);
    draw_text(48, 52, menu_sel == 0 ? "> CALENDAR" : "  CALENDAR", COLOR_WHITE, menu_sel == 0 ? COLOR_TEAL : COLOR_BLACK, 2);

    fill_rect(38, 82, 164, 25, menu_sel == 1 ? COLOR_TEAL : COLOR_BLACK);
    draw_text(48, 86, menu_sel == 1 ? "> STOPWATCH" : "  STOPWATCH", COLOR_WHITE, menu_sel == 1 ? COLOR_TEAL : COLOR_BLACK, 2);

    legend("OK", "UP", "DOWN", "BACK");
}

// ---------- CALENDAR ----------
static void draw_calendar(void) {
    char t[24], b[24], *p;
    clear_screen();

    p = put_s(t, month_name[cal_month - 1]); *p++ = ' '; p = put_u(p, cal_year, 4); *p = 0;
    center_str(b, t, 14);
    draw_text(64, 5, b, COLOR_WHITE, COLOR_BLACK, 1);
    draw_text(207, 5, "^", COLOR_CYAN, COLOR_BLACK, 1);
    draw_text(225, 5, "v", COLOR_CYAN, COLOR_BLACK, 1);

    for (int c = 0; c < 7; c++) draw_text(13 + c * 34, 25, wday_short[c], COLOR_CYAN, COLOR_BLACK, 1);
    fill_rect(5, 37, 230, 1, COLOR_DARKGRAY);

    int first = day_of_week(cal_year, cal_month, 1);
    int dim = days_in_month(cal_year, cal_month);
    int prev_dim = (cal_month == 1) ? 31 : days_in_month(cal_year, cal_month - 1);

    for (int i = 0; i < 42; i++) {
        int day; uint8_t in_month = 1;
        if (i < first) { day = prev_dim - first + i + 1; in_month = 0; }
        else if (i >= first + dim) { day = i - (first + dim) + 1; in_month = 0; }
        else { day = i - first + 1; }

        int cx = 17 + (i % 7) * 34;
        int cy = 47 + (i / 7) * 14;

        uint8_t today = (rtc.ok && in_month && cal_year == rtc.year && cal_month == rtc.month && day == rtc.date);
        uint16_t fg = in_month ? COLOR_WHITE : COLOR_GRAY;
        uint16_t bg = COLOR_BLACK;

        if (today) { tft_fill_circle(cx, cy + 4, 9, COLOR_TEAL); fg = COLOR_WHITE; bg = COLOR_TEAL; }
        char d[3]; d[0] = '0' + day/10; d[1] = '0' + day%10; d[2] = 0;
        if (d[0] == '0') d[0] = ' '; // Bo so 0 vo nghia
        draw_text(cx - 8, cy, d, fg, bg, 1);
    }
}

// ---------- STOPWATCH ----------
static void sw_reset(void) { sw_running = 0; sw_sec = 0; sw_acc = 0; sw_last_draw = 0xFFFFFFFF; }

static void stopwatch_tick(uint32_t now) {
    uint32_t delta = now - sw_last; sw_last = now;
    if (!sw_running) return;
    sw_acc += delta;
    while (sw_acc >= SystemCoreClock) {
        sw_acc -= SystemCoreClock;
        if (++sw_sec >= 6000) sw_sec = 0;
    }
}

static void draw_stopwatch(int force) {
    char b[8];
    uint32_t m = sw_sec / 60, s = sw_sec % 60;
    if (!force && sw_sec == sw_last_draw) return;
    sw_last_draw = sw_sec;

    if (force) {
        clear_screen();
        draw_text(48, 12, "STOPWATCH", COLOR_CYAN, COLOR_BLACK, 2);
        legend("RESET", "START", "STOP", "BACK");
    }
    b[0] = '0' + m / 10; b[1] = '0' + m % 10; b[2] = ':';
    b[3] = '0' + s / 10; b[4] = '0' + s % 10; b[5] = 0;
    draw_text(40, 50, b, COLOR_WHITE, COLOR_BLACK, 4);

    fill_rect(64, 100, 120, 16, COLOR_BLACK); // Xoa trang thai cu
    draw_text(80, 100, sw_running ? "RUNNING" : "STOPPED", sw_running ? COLOR_CYAN : COLOR_YELLOW, COLOR_BLACK, 1);
}

// ---------- CHUYEN MAN HINH & NUT BAM ----------
static void screen_enter(screen_t s, uint32_t now) {
    screen = s;
    switch (s) {
    case SCR_MAIN: main_dirty = 1; break;
    case SCR_MENU: draw_menu(); break;
    case SCR_CAL:  draw_calendar(); break;
    case SCR_STOPWATCH: sw_last = now; draw_stopwatch(1); break;
    }
}

static void handle_buttons(uint8_t ev, uint32_t now) {
    switch (screen) {
    case SCR_MAIN:
        if (ev & BTN_MENU) screen_enter(SCR_MENU, now); break;
    case SCR_MENU:
        if (ev & BTN_BACK) { screen_enter(SCR_MAIN, now); return; }
        if (ev & BTN_MENU) {
            if (menu_sel == 0) { cal_year = rtc.ok ? rtc.year : DEFAULT_YEAR; cal_month = rtc.ok ? rtc.month : DEFAULT_MONTH; screen_enter(SCR_CAL, now); }
            else { sw_reset(); screen_enter(SCR_STOPWATCH, now); }
            return;
        }
        if (ev & BTN_UP)   { menu_sel = 0; draw_menu(); }
        if (ev & BTN_DOWN) { menu_sel = 1; draw_menu(); }
        break;
    case SCR_CAL:
        if (ev & BTN_BACK) { screen_enter(SCR_MENU, now); return; }
        if (ev & BTN_UP)   { if (--cal_month < 1) { cal_month = 12; cal_year--; } draw_calendar(); }
        if (ev & BTN_DOWN) { if (++cal_month > 12) { cal_month = 1; cal_year++; } draw_calendar(); }
        break;
    case SCR_STOPWATCH:
        if (ev & BTN_BACK) { sw_reset(); screen_enter(SCR_MENU, now); return; }
        if (ev & BTN_MENU) { sw_reset(); draw_stopwatch(1); }
        if (ev & BTN_UP)   { if (!sw_running) { sw_running = 1; sw_last = now; } draw_stopwatch(0); }
        if (ev & BTN_DOWN) { stopwatch_tick(now); sw_running = 0; draw_stopwatch(0); }
        break;
    }
}

// ============================================================================
// HÀM MAIN - SUPER LOOP NON-BLOCKING OS
// ============================================================================
int main(void) {
    delay_ms(100);
    i2c_init();
    tft_init();

    // Thu thap I2C lan dau tien
    bh1750_start();
    delay_ms(200);
    rtc_read(&rtc);
    lux_ok = bh1750_read_lux(&lux);

    uint32_t now = get_cpu_cycles();
    rtc_last = now;
    bh_last = now;

    screen_enter(SCR_MAIN, now);

    // Bo lap lich Non-Blocking
    while (1) {
        now = get_cpu_cycles();

        // [LUONG 1]: Quet Nut Bam 10ms
        uint8_t ev = buttons_poll(now);
        if (ev) handle_buttons(ev, now);

        // [LUONG 2]: Cap nhat Stopwatch TOC DO CAO
        if (screen == SCR_STOPWATCH) {
            stopwatch_tick(now);
            draw_stopwatch(0);
        }

        // [LUONG 3]: DS3231 RTC (moi 200ms)
        if ((now - rtc_last) >= ms_to_cycles(RTC_POLL_MS)) {
            rtc_last = now;
            rtc_read(&rtc);
            // Kiem tra day co phai la luc sang giay moi hay khong de update HOME
            if (screen == SCR_MAIN && rtc.ok && rtc.sec != last_draw_sec) {
                last_draw_sec = rtc.sec;
                draw_main(); // Ve toan bo khi qua giay moi
            }
        }

        // [LUONG 4]: BH1750 (moi 1 giay do 1 lan de giam tai bus)
        if (bh_state == 0) {
            if ((now - bh_last) >= ms_to_cycles(BH_PERIOD_MS)) {
                bh_last = now;
                bh1750_start();
                bh_state = 1;
            }
        } else if ((now - bh_last) >= ms_to_cycles(BH_MEAS_MS)) {
            lux_ok = bh1750_read_lux(&lux);
            bh_state = 0;
            if (screen == SCR_MAIN && rtc.sec == last_draw_sec) {
                update_lux_only(); // Chi ve de len vung LUX ma khong load lai toan bo man hinh
            }
        }

        // Truong hop dac biet: Buoc ve lai MAIN
        if (screen == SCR_MAIN && main_dirty) {
            draw_main();
            main_dirty = 0;
            last_draw_sec = rtc.sec;
        }
    }
    return 0;
}
