#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include "LiquidCrystal_I2C.h"
#include "motor_protocol.h"
/* Arduino Uno: A0 pot; A4/A5 LCD; D1 TX -> PB11; D0 RX <- PB10. */
const uint16_t MAX_RPM = 280, ADC_LOW = 8, ADC_HIGH = 1015;
const uint8_t ADC_HYSTERESIS = 3;
LiquidCrystal_I2C lcd(0x27, 16, 2);
MP_Receiver receiver = {};
static int32_t filteredAdcQ8;
static uint16_t stableAdc, requestedRpm, confirmedSet;
static uint32_t measuredRpm10;
static uint8_t dutyPercent, motorDirection = 1, motorState = 3;
static bool haveTelemetry;
static uint32_t lastTelemetry, lastSample, lastSend, lastLcd;
static char oldRows[2][17] = {};
static uint16_t readPotAverage()
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 8; ++i) sum += analogRead(A0);
    return (uint16_t)((sum + 4u) / 8u);
}
static uint16_t adcToRpm(uint16_t adc)
{
    if (adc <= ADC_LOW) return 0;
    if (adc >= ADC_HIGH) return MAX_RPM;
    return (uint16_t)(((uint32_t)(adc - ADC_LOW) * MAX_RPM + (ADC_HIGH - ADC_LOW) / 2u) / (ADC_HIGH - ADC_LOW));
}
static void showRow(uint8_t row, const char *text)
{
    char padded[17];
    uint8_t i = 0;
    while (i < 16 && text[i]) { padded[i] = text[i]; ++i; }
    while (i < 16) padded[i++] = ' ';
    padded[16] = '\0';
    if (strcmp(padded, oldRows[row]) != 0) {
        lcd.setCursor(0, row);
        lcd.print(padded);
        memcpy(oldRows[row], padded, sizeof(padded));
    }
}
void setup()
{
    Serial.begin(9600);
    pinMode(A0, INPUT);
    lcd.init(); lcd.backlight();
    stableAdc = readPotAverage();
    filteredAdcQ8 = (int32_t)stableAdc * 256;
    requestedRpm = adcToRpm(stableAdc);
    showRow(0, "Waiting STM32..."); showRow(1, "RPM:---");
    lastSample = lastSend = lastLcd = millis();
}
void loop()
{
    while (Serial.available() > 0) {
        if (MP_Feed(&receiver, (char)Serial.read())) {
            uint32_t v[6];
            if (MP_Parse(receiver.line, 'T', v, 6) && v[0] <= MAX_RPM && v[1] <= 999990u &&
                v[2] <= 100u && v[3] <= 1u && v[4] <= 3u && v[5] <= 32768u) {
                confirmedSet = (uint16_t)v[0]; measuredRpm10 = v[1];
                dutyPercent = (uint8_t)v[2]; motorDirection = (uint8_t)v[3]; motorState = (uint8_t)v[4];
                lastTelemetry = millis(); haveTelemetry = true;
            }
        }
    }
    uint32_t now = millis();
    if ((uint32_t)(now - lastSample) >= 10u) {
        lastSample = now;
        int32_t sample = (int32_t)readPotAverage() * 256;
        filteredAdcQ8 += (sample - filteredAdcQ8) / 4;
        uint16_t adc = (uint16_t)((filteredAdcQ8 + 128) / 256);
        int diff = (int)adc - (int)stableAdc;
        if (diff >= ADC_HYSTERESIS || diff <= -(int)ADC_HYSTERESIS || adc <= ADC_LOW || adc >= ADC_HIGH) {
            stableAdc = adc; requestedRpm = adcToRpm(stableAdc);
        }
    }
    if ((uint32_t)(now - lastSend) >= 100u) {
        lastSend = now;
        char payload[16], frame[24];
        snprintf(payload, sizeof(payload), "S,%u", (unsigned)requestedRpm);
        size_t n = MP_Encode(frame, sizeof(frame), payload);
        if (n && Serial.availableForWrite() >= (int)n) Serial.write((const uint8_t *)frame, n);
    }
    if ((uint32_t)(now - lastLcd) >= 200u) {
        lastLcd = now;
        char row[17];
        if (!haveTelemetry || (uint32_t)(now - lastTelemetry) > 1000u) {
            snprintf(row, sizeof(row), "Req:%3u NO LINK", (unsigned)requestedRpm);
            showRow(0, row); showRow(1, "RPM:--- P:---");
        } else {
            const char *state = motorState == 3 ? "LINK" : motorState == 2 ? "WAIT" : motorState == 1 ? "RUN" : "STOP";
            snprintf(row, sizeof(row), "Set:%3u %s %s", (unsigned)confirmedSet, motorDirection ? "FWD" : "REV", state);
            showRow(0, row);
            if (measuredRpm10 > 9999u) showRow(1, "RPM:OVER RANGE");
            else {
                snprintf(row, sizeof(row), "RPM:%3u.%u P:%3u", (unsigned)(measuredRpm10 / 10u),
                    (unsigned)(measuredRpm10 % 10u), (unsigned)dutyPercent);
                showRow(1, row);
            }
        }
    }
}
