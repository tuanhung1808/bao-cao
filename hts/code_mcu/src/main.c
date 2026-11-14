/*
 * main.c - Gowin PicoRV32 / Tang Nano 9K
 * TFT ST7789 + DS3231 + BH1750 + 4 GPIO buttons
 * HOME chi cap nhat cac vung thay doi, khong clear toan man hinh moi giay.
 * STOPWATCH hien thi MM:SS:CC (CC = 1/100 giay).
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
volatile uint32_t irq00_flag = 0;
volatile uint32_t irq13_flag = 0;
volatile uint32_t irq20_flag = 0;
volatile uint32_t irq21_flag = 0;

/* ---------------- Hardware configuration ---------------- */
#define BH1750_ADDR       0x23
#define DS3231_ADDR       0x68
#define I2C_TIMEOUT       1000000UL
#define GPIOP             PICO_WBGPIO
#define SPIP              PICO_WBSPI_MASTER

#define DC                (1u << 0)  /* GPIO bit 0: TFT DC */
#define RES               (1u << 1)  /* GPIO bit 1: TFT RESET */
#define SCR_W             240
#define SCR_H             135
#define X_OFF             40
#define Y_OFF             52

#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_CYAN        0x07FF
#define COLOR_TEAL        0x1D6A
#define COLOR_GRAY        0x8410
#define COLOR_YELLOW      0xFFE0
#define COLOR_DARKGRAY    0x4208

#define DEFAULT_YEAR      2026
#define DEFAULT_MONTH     11

#define RTC_POLL_MS       200
#define BH_PERIOD_MS      1000
#define BH_MEAS_MS        180
#define BTN_SCAN_MS       5
#define BTN_DEBOUNCE_SAMPLES 3
#define SW_TICK_HZ        100U

/* Button event bits after converting active-low GPIO inputs. */
#define BTN_MENU          0x01
#define BTN_UP            0x02
#define BTN_DOWN          0x04
#define BTN_BACK          0x08

typedef struct {
    int      ok;
    uint8_t  sec, min, hour, date, month;
    uint16_t year;
} rtc_t;

typedef enum { SCR_MAIN, SCR_MENU, SCR_CAL, SCR_STOPWATCH } screen_t;

/* ---------------- 8x8 ASCII font ---------------- */
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

/* ---------------- Utility functions ---------------- */
static uint32_t get_cpu_cycles(void)
{
    uint32_t cycles;
    __asm__ volatile ("rdcycle %0" : "=r" (cycles));
    return cycles;
}

static uint32_t ms_to_cycles(uint32_t ms)
{
    return ms * (SystemCoreClock / 1000U);
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = get_cpu_cycles();
    uint32_t wait = ms_to_cycles(ms);
    while ((uint32_t)(get_cpu_cycles() - start) < wait) { }
}

static uint8_t bcd_to_dec(uint8_t value)
{
    return (uint8_t)(((value >> 4) * 10U) + (value & 0x0FU));
}

static char *put_u(char *p, uint32_t value, int min_digits)
{
    char temp[10];
    int n = 0;
    do {
        temp[n++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U && n < (int)sizeof(temp));
    while (n < min_digits && n < (int)sizeof(temp)) temp[n++] = '0';
    while (n > 0) *p++ = temp[--n];
    return p;
}

static char *put_s(char *p, const char *s)
{
    while (*s) *p++ = *s++;
    return p;
}

static int str_len(const char *s)
{
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void center_str(char *dst, const char *src, int width)
{
    int len = str_len(src);
    int left = (width - len) / 2;
    int i = 0;
    if (left < 0) left = 0;
    while (i < left && i < width) dst[i++] = ' ';
    for (int j = 0; src[j] && i < width; j++) dst[i++] = src[j];
    while (i < width) dst[i++] = ' ';
    dst[i] = 0;
}

/* ---------------- Calendar calculations ---------------- */
static int is_leap(int year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

static int days_in_month(int year, int month)
{
    static const uint8_t days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && is_leap(year)) return 29;
    return days[month - 1];
}

/* Sunday = 0 ... Saturday = 6 */
static int day_of_week(int year, int month, int day)
{
    static const int offsets[12] = {0,3,2,5,0,3,5,1,4,6,2,4};
    if (month < 3) year--;
    return (year + year/4 - year/100 + year/400 + offsets[month - 1] + day) % 7;
}

static const char *const month_name[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};
static const char *const wday_name[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *const wday_short[7] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

/* ---------------- GPIO and button handling ---------------- */
static uint32_t gpio_out = 0;

static void gpio_set(uint32_t mask)
{
    gpio_out |= mask;
    GPIO_WriteData(GPIOP, gpio_out);
}

static void gpio_clr(uint32_t mask)
{
    gpio_out &= ~mask;
    GPIO_WriteData(GPIOP, gpio_out);
}

/* GPIO bits 2..5 are active-low buttons. Return 1 when pressed. */
static uint8_t read_buttons_pressed(void)
{
    uint32_t input = GPIO_ReadData(GPIOP);
    return (uint8_t)((~input >> 2) & 0x0FU);
}

/*
 * Debounce scanner + small event queue.
 * The scanner is serviced both from the super-loop and during SPI writes,
 * so a long TFT redraw does not make button presses disappear.
 */
#define BUTTON_QUEUE_SIZE 8U
static uint8_t button_queue[BUTTON_QUEUE_SIZE];
static uint8_t button_q_head = 0;
static uint8_t button_q_tail = 0;
static uint32_t button_last_scan = 0;
static uint8_t button_stable = 0;

static void buttons_service(void)
{
    uint32_t now = get_cpu_cycles();
    uint8_t raw;

    /* ĐÃ FIX: Bulletproof Debounce (Khóa 50ms chống nhiễu phím cứng) */
    if ((uint32_t)(now - button_last_scan) < ms_to_cycles(50)) return;

    raw = read_buttons_pressed();
    if (raw != button_stable) {
        uint8_t events = (uint8_t)(raw & (uint8_t)(~button_stable));
        button_stable = raw;
        button_last_scan = now; /* Khóa chết bộ lọc trong 50ms tiếp theo */

        if (events != 0U) {
            uint8_t next = (uint8_t)((button_q_head + 1U) & (BUTTON_QUEUE_SIZE - 1U));
            if (next != button_q_tail) {
                button_queue[button_q_head] = events;
                button_q_head = next;
            }
        }
    }
}

static uint8_t buttons_get_event(void)
{
    uint8_t event;
    if (button_q_tail == button_q_head) return 0;
    event = button_queue[button_q_tail];
    button_q_tail = (uint8_t)((button_q_tail + 1U) & (BUTTON_QUEUE_SIZE - 1U));
    return event;
}

/* ---------------- I2C: DS3231 + BH1750 ---------------- */
static int i2c_wait_tip(void)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (PICO_WBI2C->SR & I2C_SR_TIP) {
        if (timeout == 0) return 0;
        timeout--;
    }
    return 1;
}

static int i2c_wait_idle(void)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (PICO_WBI2C->SR & I2C_SR_BUSY) {
        if (timeout == 0) return 0;
        timeout--;
    }
    return 1;
}

static void i2c_stop(void)
{
    PICO_WBI2C->CR = I2C_CMD_STO;
    (void)i2c_wait_tip();
    (void)i2c_wait_idle();
}

static int i2c_write_byte(uint8_t data, int start, int stop)
{
    uint32_t command = I2C_CMD_WR;
    PICO_WBI2C->TXR = data;
    if (start) command |= I2C_CMD_STA;
    if (stop) command |= I2C_CMD_STO;
    PICO_WBI2C->CR = command;
    if (!i2c_wait_tip()) return -1;
    if (PICO_WBI2C->SR & I2C_SR_RXACK) return 0;
    return 1;
}

static void i2c_init(void)
{
    PICO_WBI2C->CHANNEL = I2C_CHANNEL_0;
    PICO_WBI2C->PRER = 53;
    PICO_WBI2C->CTR = I2C_CTR_EN;
    delay_ms(10);
}

static void bh1750_start(void)
{
    if (i2c_write_byte((uint8_t)(BH1750_ADDR << 1), 1, 0) == 1) {
        if (i2c_write_byte(0x01, 0, 1) != 1) i2c_stop();
    } else {
        i2c_stop();
        return;
    }
    delay_ms(5);
    if (i2c_write_byte((uint8_t)(BH1750_ADDR << 1), 1, 0) == 1) {
        if (i2c_write_byte(0x20, 0, 1) != 1) i2c_stop();
    } else {
        i2c_stop();
    }
}

static int bh1750_read_lux(uint32_t *lux)
{
    if (i2c_write_byte((uint8_t)((BH1750_ADDR << 1) | 1U), 1, 0) != 1) {
        i2c_stop();
        return 0;
    }
    PICO_WBI2C->CR = I2C_CMD_RD;
    if (!i2c_wait_tip()) { i2c_stop(); return 0; }
    uint8_t msb = (uint8_t)(PICO_WBI2C->RXR & 0xFFU);

    PICO_WBI2C->CR = I2C_CMD_RD | I2C_CMD_ACK | I2C_CMD_STO;
    if (!i2c_wait_tip()) { i2c_stop(); return 0; }
    uint8_t lsb = (uint8_t)(PICO_WBI2C->RXR & 0xFFU);
    if (!i2c_wait_idle()) return 0;

    uint32_t raw = ((uint32_t)msb << 8) | lsb;
    *lux = (raw * 10U + 6U) / 12U;
    return 1;
}

static int ds3231_read_regs(uint8_t reg, uint8_t *buffer, uint32_t length)
{
    if (buffer == 0 || length == 0) return 0;
    if (i2c_write_byte((uint8_t)(DS3231_ADDR << 1), 1, 0) != 1) { i2c_stop(); return -1; }
    if (i2c_write_byte(reg, 0, 0) != 1) { i2c_stop(); return -1; }
    if (i2c_write_byte((uint8_t)((DS3231_ADDR << 1) | 1U), 1, 0) != 1) { i2c_stop(); return -1; }

    for (uint32_t i = 0; i < length; i++) {
        PICO_WBI2C->CR = (i == length - 1U)
            ? (I2C_CMD_RD | I2C_CMD_ACK | I2C_CMD_STO)
            : I2C_CMD_RD;
        if (!i2c_wait_tip()) { i2c_stop(); return -1; }
        buffer[i] = (uint8_t)(PICO_WBI2C->RXR & 0xFFU);
    }
    if (!i2c_wait_idle()) return -1;
    return 1;
}

static void rtc_read(rtc_t *rtc)
{
    uint8_t b[7];
    rtc->ok = (ds3231_read_regs(0x00, b, 7) == 1);
    if (!rtc->ok) return;

    rtc->sec = bcd_to_dec(b[0] & 0x7F);
    rtc->min = bcd_to_dec(b[1] & 0x7F);

    /* Support both DS3231 24-hour and 12-hour formats. */
    if (b[2] & 0x40U) {
        uint8_t hour12 = bcd_to_dec(b[2] & 0x1FU);
        if (b[2] & 0x20U) {             /* PM */
            rtc->hour = (hour12 == 12U) ? 12U : (uint8_t)(hour12 + 12U);
        } else {                        /* AM */
            rtc->hour = (hour12 == 12U) ? 0U : hour12;
        }
    } else {
        rtc->hour = bcd_to_dec(b[2] & 0x3FU);
    }

    rtc->date = bcd_to_dec(b[4] & 0x3FU);
    rtc->month = bcd_to_dec(b[5] & 0x1FU);
    rtc->year = (uint16_t)(2000U + bcd_to_dec(b[6]) + ((b[5] & 0x80U) ? 100U : 0U));

    if (rtc->month < 1U || rtc->month > 12U || rtc->date < 1U || rtc->date > 31U ||
        rtc->hour > 23U || rtc->min > 59U || rtc->sec > 59U) rtc->ok = 0;
}

/* ---------------- TFT ST7789 ---------------- */
static void tft_cs_low(void)  { SPIP->SSMASK = 1; }
static void tft_cs_high(void) { SPIP->SSMASK = 0; }
static void spi_send(uint8_t data)
{
    static uint8_t poll_divider = 0;
    wbspi_master_txdata(SPIP, data);
    /* Check the GPIO input regularly while a screen is being transferred. */
    poll_divider++;
    if (poll_divider >= 32U) {
        poll_divider = 0;
        buttons_service();
    }
}

static void tft_cmd(uint8_t command)
{
    tft_cs_high();
    for (volatile int w = 0; w < 10; w++) __asm__("nop");
    gpio_clr(DC);
    tft_cs_low();
    spi_send(command);
}

static void tft_dat(uint8_t data)
{
    gpio_set(DC);
    spi_send(data);
}

static void window(int x0, int y0, int x1, int y1)
{
    x0 += X_OFF; x1 += X_OFF;
    y0 += Y_OFF; y1 += Y_OFF;
    tft_cmd(0x2A);
    tft_dat((uint8_t)(x0 >> 8)); tft_dat((uint8_t)x0);
    tft_dat((uint8_t)(x1 >> 8)); tft_dat((uint8_t)x1);
    tft_cmd(0x2B);
    tft_dat((uint8_t)(y0 >> 8)); tft_dat((uint8_t)y0);
    tft_dat((uint8_t)(y1 >> 8)); tft_dat((uint8_t)y1);
    tft_cmd(0x2C);
    gpio_set(DC);
}

static void tft_init(void)
{
    GPIO_Init(GPIOP);
    gpio_out = 0;
    GPIO_WriteData(GPIOP, 0);
    /* Keep the tested GPIO API: bits 0/1 output; bits 2..5 remain inputs. */
    GPIO_SetDir(GPIOP, DC | RES);

    tft_cs_high();
    gpio_set(RES); delay_ms(50);
    gpio_clr(RES); delay_ms(150);
    gpio_set(RES); delay_ms(150);
    tft_cmd(0x11); delay_ms(150);
    tft_cmd(0x36); tft_dat(0x70);
    tft_cmd(0x3A); tft_dat(0x05);
    tft_cmd(0x21);
    tft_cmd(0x13);
    tft_cmd(0x29);
    delay_ms(50);
}

static void send_color(uint16_t color)
{
    spi_send((uint8_t)(color >> 8));
    spi_send((uint8_t)color);
}

static void fill_rect(int x, int y, int width, int height, uint16_t color)
{
    if (width <= 0 || height <= 0) return;
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > SCR_W) width = SCR_W - x;
    if (y + height > SCR_H) height = SCR_H - y;
    if (width <= 0 || height <= 0) return;

    window(x, y, x + width - 1, y + height - 1);
    tft_cs_low();
    for (uint32_t i = 0; i < (uint32_t)width * (uint32_t)height; i++) send_color(color);
    tft_cs_high();
}

static void clear_screen(void)
{
    fill_rect(0, 0, SCR_W, SCR_H, COLOR_BLACK);
}

static void tft_fill_circle(int cx, int cy, int radius, uint16_t color)
{
    int y, x;
    if (radius <= 0) {
        fill_rect(cx, cy, 1, 1, color);
        return;
    }

    /* One TFT address window instead of one window per scanline. */
    window(cx - radius, cy - radius, cx + radius, cy + radius);
    tft_cs_low();
    for (y = -radius; y <= radius; y++) {
        for (x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) send_color(color);
            else send_color(COLOR_BLACK);
        }
    }
    tft_cs_high();
}

/*
 * Fast text renderer: one CASET/RASET/RAMWR window for the whole string.
 * The old renderer opened a new TFT window for every character, which
 * generated many SPI commands and made page transitions visibly slow.
 */
static void draw_text(int x, int y, const char *text, uint16_t fg, uint16_t bg, int scale)
{
    int length;
    int visible_chars;
    int pixel_y;
    int ch;
    int pixel_x;

    if (text == 0 || scale <= 0 || x < 0 || y < 0) return;
    if (y + 8 * scale > SCR_H || x >= SCR_W) return;

    length = str_len(text);
    if (length <= 0) return;

    visible_chars = (SCR_W - x) / (8 * scale);
    if (length > visible_chars) length = visible_chars;
    if (length <= 0) return;

    window(x, y, x + length * 8 * scale - 1, y + 8 * scale - 1);
    tft_cs_low();

    for (pixel_y = 0; pixel_y < 8 * scale; pixel_y++) {
        int font_row = pixel_y / scale;
        for (ch = 0; ch < length; ch++) {
            unsigned char c = (unsigned char)text[ch];
            uint8_t row_bits;
            if (c < 32U || c > 126U) c = '?';
            row_bits = font[c - 32U][font_row];

            for (pixel_x = 0; pixel_x < 8 * scale; pixel_x++) {
                int bit = 7 - (pixel_x / scale);
                send_color((row_bits & (1U << bit)) ? fg : bg);
            }
        }
    }

    tft_cs_high();
}

/* ---------------- Application state ---------------- */
static screen_t screen = SCR_MAIN;
static int menu_sel = 0;
static rtc_t rtc;
static int lux_ok = 0;
static uint32_t lux = 0;
static int main_dirty = 1;
static uint32_t last_draw_sec = 0xFFFFFFFFUL;

static uint32_t rtc_last = 0;
static uint32_t bh_last = 0;
static int bh_state = 0;

static int cal_year = DEFAULT_YEAR;
static int cal_month = DEFAULT_MONTH;

/* Stopwatch stores total centiseconds. sw_acc stores remaining CPU cycles. */
static int sw_running = 0;
static uint32_t sw_centiseconds = 0;
static uint32_t sw_acc = 0;
static uint32_t sw_last = 0;
static uint32_t sw_last_draw = 0xFFFFFFFFUL;

static void legend(const char *a, const char *b, const char *c, const char *d)
{
    const char *items[4] = {a, b, c, d};
    for (int i = 0; i < 4; i++) {
        int len = str_len(items[i]);
        if (len > 0) draw_text(i * 60 + (60 - len * 8) / 2, 124, items[i], COLOR_GRAY, COLOR_BLACK, 1);
    }
}

/* ---------------- HOME: draw static once, then update only changing fields ---------------- */
static void draw_home_time_fields(void)
{
    char buf[32];
    char *p;

    /* draw_text() paints the background inside each glyph cell, so fixed-width
       fields can be updated directly without clearing the whole screen. */
    if (rtc.ok) {
        p = put_u(buf, rtc.hour, 2); *p++ = ':';
        p = put_u(p, rtc.min, 2);    *p++ = ':';
        p = put_u(p, rtc.sec, 2);    *p = 0;
        draw_text(22, 54, buf, COLOR_WHITE, COLOR_BLACK, 3);

        p = put_u(buf, rtc.date, 2); *p++ = '/';
        p = put_u(p, rtc.month, 2);  *p++ = '/';
        p = put_u(p, rtc.year, 4);   *p++ = ' '; *p++ = ' ';
        p = put_s(p, wday_name[day_of_week(rtc.year, rtc.month, rtc.date)]);
        *p = 0;
        draw_text(30, 91, buf, COLOR_WHITE, COLOR_BLACK, 1);
    } else {
        fill_rect(18, 50, 210, 31, COLOR_BLACK);
        fill_rect(24, 88, 210, 14, COLOR_BLACK);
        draw_text(22, 54, "RTC ERROR", COLOR_YELLOW, COLOR_BLACK, 2);
    }
}

static void update_lux_only(void)
{
    char buf[16];
    char *p;
    fill_rect(64, 116, 168, 16, COLOR_BLACK);
    if (lux_ok) {
        p = put_u(buf, lux, 1);
        p = put_s(p, " lx");
        *p = 0;
        draw_text(66, 119, buf, COLOR_CYAN, COLOR_BLACK, 1);
    } else {
        draw_text(66, 119, "ERR", COLOR_YELLOW, COLOR_BLACK, 1);
    }
}

static void draw_main(void)
{
    clear_screen();
    draw_text(8, 8, "HOME", COLOR_CYAN, COLOR_BLACK, 2);
    draw_text(8, 40, "TIME", COLOR_GRAY, COLOR_BLACK, 1);
    fill_rect(8, 111, 224, 1, COLOR_DARKGRAY);
    draw_text(10, 119, "LIGHT", COLOR_GRAY, COLOR_BLACK, 1);
    draw_home_time_fields();
    update_lux_only();
}

/* ---------------- MENU ---------------- */
static void draw_menu(void)
{
    clear_screen();
    draw_text(88, 12, "MENU", COLOR_CYAN, COLOR_BLACK, 2);

    fill_rect(38, 48, 164, 25, menu_sel == 0 ? COLOR_TEAL : COLOR_BLACK);
    draw_text(48, 52, menu_sel == 0 ? "> CALENDAR" : "  CALENDAR",
              COLOR_WHITE, menu_sel == 0 ? COLOR_TEAL : COLOR_BLACK, 2);

    fill_rect(38, 82, 164, 25, menu_sel == 1 ? COLOR_TEAL : COLOR_BLACK);
    draw_text(48, 86, menu_sel == 1 ? "> STOPWATCH" : "  STOPWATCH",
              COLOR_WHITE, menu_sel == 1 ? COLOR_TEAL : COLOR_BLACK, 2);
    legend("OK", "UP", "DOWN", "BACK");
}

/* ---------------- CALENDAR ---------------- */
static void draw_calendar(void)
{
    char title[24];
    char centered[24];
    char *p;

    clear_screen();
    p = put_s(title, month_name[cal_month - 1]);
    *p++ = ' ';
    p = put_u(p, (uint32_t)cal_year, 4);
    *p = 0;
    center_str(centered, title, 14);
    draw_text(64, 5, centered, COLOR_WHITE, COLOR_BLACK, 1);
    draw_text(207, 5, "^", COLOR_CYAN, COLOR_BLACK, 1);
    draw_text(225, 5, "v", COLOR_CYAN, COLOR_BLACK, 1);

    for (int col = 0; col < 7; col++)
        draw_text(13 + col * 34, 25, wday_short[col], COLOR_CYAN, COLOR_BLACK, 1);
    fill_rect(5, 37, 230, 1, COLOR_DARKGRAY);

    int first = day_of_week(cal_year, cal_month, 1);
    int dim = days_in_month(cal_year, cal_month);
    int prev_dim = (cal_month == 1) ? days_in_month(cal_year - 1, 12) : days_in_month(cal_year, cal_month - 1);

    for (int i = 0; i < 42; i++) {
        int day;
        int in_month = 1;
        if (i < first) {
            day = prev_dim - first + i + 1;
            in_month = 0;
        } else if (i >= first + dim) {
            day = i - (first + dim) + 1;
            in_month = 0;
        } else {
            day = i - first + 1;
        }

        int cx = 17 + (i % 7) * 34;
        int cy = 47 + (i / 7) * 14;
        int today = rtc.ok && in_month && cal_year == rtc.year && cal_month == rtc.month && day == rtc.date;
        uint16_t fg = in_month ? COLOR_WHITE : COLOR_GRAY;
        uint16_t bg = COLOR_BLACK;

        if (today) {
            tft_fill_circle(cx, cy + 4, 9, COLOR_TEAL);
            fg = COLOR_WHITE;
            bg = COLOR_TEAL;
        }

        char day_text[3];
        day_text[0] = (char)('0' + day / 10);
        day_text[1] = (char)('0' + day % 10);
        day_text[2] = 0;
        if (day_text[0] == '0') day_text[0] = ' ';
        draw_text(cx - 8, cy, day_text, fg, bg, 1);
    }
}

/* ---------------- STOPWATCH: MM:SS:CC ---------------- */
static void sw_reset(void)
{
    sw_running = 0;
    sw_centiseconds = 0;
    sw_acc = 0;
    sw_last_draw = 0xFFFFFFFFUL;
}

static void stopwatch_tick(uint32_t now)
{
    uint32_t delta = (uint32_t)(now - sw_last);
    sw_last = now;
    if (!sw_running) return;

    sw_acc += delta;
    const uint32_t cycles_per_tick = SystemCoreClock / SW_TICK_HZ;
    while (sw_acc >= cycles_per_tick) {
        sw_acc -= cycles_per_tick;
        sw_centiseconds++;
        /* Keep two minute digits: roll over after 99:59.99. */
        if (sw_centiseconds >= 600000UL) sw_centiseconds = 0;
    }
}

static void draw_sw_status(void)
{
    fill_rect(60, 91, 120, 15, COLOR_BLACK);
    draw_text(80, 94, sw_running ? "RUNNING" : "STOPPED",
              sw_running ? COLOR_CYAN : COLOR_YELLOW, COLOR_BLACK, 1);
}

static void draw_stopwatch(int force)
{
    char value[9];
    char mmss[6];
    char cc[3];
    uint32_t minutes = sw_centiseconds / 6000U;
    uint32_t seconds = (sw_centiseconds / 100U) % 60U;
    uint32_t ticks = sw_centiseconds % 100U;
    uint32_t old_value = sw_last_draw;
    int first_draw = (old_value == 0xFFFFFFFFUL);
    int second_changed = first_draw || ((sw_centiseconds / 100U) != (old_value / 100U));

    value[0] = (char)('0' + (minutes / 10U) % 10U);
    value[1] = (char)('0' + minutes % 10U);
    value[2] = ':';
    value[3] = (char)('0' + seconds / 10U);
    value[4] = (char)('0' + seconds % 10U);
    value[5] = ':';
    value[6] = (char)('0' + ticks / 10U);
    value[7] = (char)('0' + ticks % 10U);
    value[8] = 0;

    mmss[0] = value[0]; mmss[1] = value[1]; mmss[2] = value[2];
    mmss[3] = value[3]; mmss[4] = value[4]; mmss[5] = 0;
    cc[0] = value[6]; cc[1] = value[7]; cc[2] = 0;

    if (force) {
        clear_screen();
        draw_text(48, 12, "STOPWATCH", COLOR_CYAN, COLOR_BLACK, 2);
        legend("RESET", "START", "STOP", "BACK");
        draw_text(24, 52, value, COLOR_WHITE, COLOR_BLACK, 3);
        draw_sw_status();
        sw_last_draw = sw_centiseconds;
        return;
    }

    /* Update only changed fields. Never blank the whole stopwatch area. */
    if (second_changed) {
        draw_text(24, 52, mmss, COLOR_WHITE, COLOR_BLACK, 3);
    }
    if (first_draw || sw_centiseconds != old_value) {
        draw_text(24 + 6 * 24, 52, cc, COLOR_WHITE, COLOR_BLACK, 3);
    }

    sw_last_draw = sw_centiseconds;
}

/* ---------------- Screen transitions and button actions ---------------- */
static void screen_enter(screen_t next, uint32_t now)
{
    screen = next;
    switch (next) {
    case SCR_MAIN:
        main_dirty = 1;
        break;
    case SCR_MENU:
        draw_menu();
        break;
    case SCR_CAL:
        draw_calendar();
        break;
    case SCR_STOPWATCH:
        sw_last = now;
        draw_stopwatch(1);
        break;
    }
}

static void handle_buttons(uint8_t events, uint32_t now)
{
    switch (screen) {
    case SCR_MAIN:
        if (events & BTN_MENU) screen_enter(SCR_MENU, now);
        /* ĐÃ FIX: Cho phép bấm BACK ở màn hình chính mở Menu */
        if (events & BTN_BACK) screen_enter(SCR_MENU, now);
        break;

    case SCR_MENU:
        if (events & BTN_BACK) {
            screen_enter(SCR_MAIN, now);
            return;
        }
        if (events & BTN_MENU) {
            if (menu_sel == 0) {
                cal_year = rtc.ok ? rtc.year : DEFAULT_YEAR;
                cal_month = rtc.ok ? rtc.month : DEFAULT_MONTH;
                screen_enter(SCR_CAL, now);
            } else {
                sw_reset();
                screen_enter(SCR_STOPWATCH, now);
            }
            return;
        }
        if (events & BTN_UP) { menu_sel = 0; draw_menu(); }
        if (events & BTN_DOWN) { menu_sel = 1; draw_menu(); }
        break;

    case SCR_CAL:
        if (events & BTN_BACK) {
            screen_enter(SCR_MENU, now);
            return;
        }
        if (events & BTN_UP) {
            if (--cal_month < 1) { cal_month = 12; cal_year--; }
            draw_calendar();
        }
        if (events & BTN_DOWN) {
            if (++cal_month > 12) { cal_month = 1; cal_year++; }
            draw_calendar();
        }
        break;

    case SCR_STOPWATCH:
        if (events & BTN_BACK) {
            sw_reset();
            screen_enter(SCR_MENU, now);
            return;
        }
        if (events & BTN_MENU) {
            sw_reset();
            sw_last = now;
            draw_stopwatch(0);
            draw_sw_status();
        }
        if (events & BTN_UP) {
            if (!sw_running) {
                sw_running = 1;
                sw_last = now;
                draw_sw_status();
            }
        }
        if (events & BTN_DOWN) {
            stopwatch_tick(now);
            sw_running = 0;
            draw_stopwatch(0);
            draw_sw_status();
        }
        break;
    }
}

/* ---------------- Main non-blocking super loop ---------------- */
int main(void)
{
    delay_ms(100);
    i2c_init();
    tft_init();

    /* First sensor readings before entering HOME. */
    bh1750_start();
    delay_ms(200);
    rtc_read(&rtc);
    lux_ok = bh1750_read_lux(&lux);

    uint32_t now = get_cpu_cycles();
    rtc_last = now;
    bh_last = now;
    screen_enter(SCR_MAIN, now);

    while (1) {
        now = get_cpu_cycles();

        /*
         * Service GPIO buttons here and inside spi_send().
         * Events captured during a long TFT redraw are queued and handled
         * immediately after the current transfer finishes.
         */
        buttons_service();
        {
            uint8_t events;
            while ((events = buttons_get_event()) != 0U) {
                now = get_cpu_cycles();
                handle_buttons(events, now);
            }
        }

        /* Stopwatch timebase runs from CPU cycle counter. */
        if (screen == SCR_STOPWATCH) {
            stopwatch_tick(now);
            draw_stopwatch(0);
        }

        /* Read RTC every 200ms; repaint only the time/date field on HOME. */
        if ((uint32_t)(now - rtc_last) >= ms_to_cycles(RTC_POLL_MS)) {
            rtc_last = now;
            rtc_read(&rtc);
            if (screen == SCR_MAIN && rtc.ok && rtc.sec != last_draw_sec) {
                last_draw_sec = rtc.sec;
                draw_home_time_fields();
            }
        }

        /* BH1750: start one-shot measurement, then read after conversion time. */
        if (bh_state == 0) {
            if ((uint32_t)(now - bh_last) >= ms_to_cycles(BH_PERIOD_MS)) {
                bh_last = now;
                bh1750_start();
                bh_state = 1;
            }
        } else if ((uint32_t)(now - bh_last) >= ms_to_cycles(BH_MEAS_MS)) {
            lux_ok = bh1750_read_lux(&lux);
            bh_state = 0;
            if (screen == SCR_MAIN) update_lux_only();
        }

        /* Draw the HOME screen only on entry, not once per second. */
        if (screen == SCR_MAIN && main_dirty) {
            draw_main();
            main_dirty = 0;
            last_draw_sec = rtc.ok ? rtc.sec : 0xFFFFFFFFUL;
        }
    }
    return 0;
}
