#ifndef MOTOR_PROTOCOL_H
#define MOTOR_PROTOCOL_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Command: $S,<rpm>*<CRC8>\n
 * Reply: $T,<set>,<rpm_x10>,<duty_pct>,<dir>,<state>,<sample_counts>*<CRC8>\n
 * CRC-8/SMBUS (poly 0x07, init 0, no reflection/final xor), payload only.
 * dir: 1 forward, 0 reverse; state: 0 stop, 1 run, 2 reversing, 3 link lost.
 */
#define MP_CAPACITY 64u
typedef struct { char line[MP_CAPACITY]; uint8_t length, active; } MP_Receiver;
static inline uint8_t MP_CRC(const char *p, size_t n)
{
    uint8_t crc = 0;
    while (n--) {
        crc ^= (uint8_t)*p++;
        for (unsigned i = 0; i < 8; ++i)
            crc = (uint8_t)((crc & 0x80u) ? ((unsigned)crc << 1) ^ 0x07u : (unsigned)crc << 1);
    }
    return crc;
}
static inline int MP_Hex(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
static inline void MP_Reset(MP_Receiver *r) { r->length = 0; r->active = 0; }
/* Consume the returned payload immediately, before feeding another byte. */
static inline int MP_Feed(MP_Receiver *r, char c)
{
    if (c == '$') { r->length = 0; r->active = 1; return 0; }
    if (!r->active) return 0;
    if (c == '\n') {
        unsigned n = r->length;
        r->active = 0;
        if (n && r->line[n - 1] == '\r') --n;
        if (n < 4 || r->line[n - 3] != '*') return 0;
        int hi = MP_Hex(r->line[n - 2]), lo = MP_Hex(r->line[n - 1]);
        if (hi < 0 || lo < 0) return 0;
        if (MP_CRC(r->line, n - 3) != (uint8_t)((hi << 4) | lo)) return 0;
        r->line[n - 3] = '\0';
        return 1;
    }
    if (((uint8_t)c < 32u && c != '\r') || (uint8_t)c > 126u) { MP_Reset(r); return 0; }
    if (r->length >= MP_CAPACITY - 1u) { MP_Reset(r); return 0; }
    r->line[r->length++] = c;
    return 0;
}
static inline size_t MP_Encode(char *dst, size_t cap, const char *payload)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t n = strlen(payload);
    if (!n || n + 6 > cap) return 0;
    uint8_t crc = MP_CRC(payload, n);
    dst[0] = '$'; memcpy(dst + 1, payload, n);
    dst[n + 1] = '*'; dst[n + 2] = hex[crc >> 4]; dst[n + 3] = hex[crc & 15u];
    dst[n + 4] = '\n'; dst[n + 5] = '\0';
    return n + 5;
}
/* Exact tag, field count and unsigned decimal syntax; rejects overflow. */
static inline int MP_Parse(const char *p, char tag, uint32_t *values, unsigned count)
{
    if (*p++ != tag || *p++ != ',' || !count) return 0;
    for (unsigned i = 0; i < count; ++i) {
        uint32_t v = 0;
        if (*p < '0' || *p > '9') return 0;
        do {
            unsigned digit = (unsigned)(*p++ - '0');
            if (v > (UINT32_MAX - digit) / 10u) return 0;
            v = v * 10u + digit;
        } while (*p >= '0' && *p <= '9');
        values[i] = v;
        if (i + 1u < count) { if (*p++ != ',') return 0; }
        else if (*p != '\0') return 0;
    }
    return 1;
}
#endif
