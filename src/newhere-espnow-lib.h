#pragma once

// newhere-espnow-lib
// Shared message structs and framed-UART transport for the NewHere beacon link.
//
// Message structs use fixed-width types so the wire layout is identical on
// every supported MCU (ESP32, Teensy 4.1). Both are little-endian with
// IEEE-754 floats and every member is 4 bytes, so there is no struct padding.
//
// Two transports carry the same payloads:
//   - ESP-NOW: send the struct bytes directly (packet boundaries are native)
//   - UART (ESP32 <-> Teensy): prefix each payload with the 4-byte frame
//     header [0xA5][0x5A][len_lo][len_hi] so a stream receiver can find
//     message boundaries.

#include <stdint.h>

typedef struct Joystick_Msg
{
    float    x;
    float    y;
    int32_t  c;
    int32_t  z;
    uint32_t timestamp;
} Joystick_Msg;

typedef struct Power_Msg
{
    float voltage;
    float current;
} Power_Msg;

typedef struct Position_Msg
{
    float    lat;       // decimal degrees
    float    lon;       // decimal degrees
    float    alt;       // meters
    float    acc;       // horizontal accuracy in millimeters
    int32_t  rtk;       // 0: no RTK solution, 1: float solution, 2: fixed solution
    uint32_t unixtime;  // time, referencing Unix epoch rather than GPS
} Position_Msg;

// UART frame header. Wire order: 0xA5, 0x5A, then 16-bit payload length in
// little-endian byte order, followed by that many payload bytes.
#define SERIAL_FRAME_SYNC    0xA55A
#define SERIAL_FRAME_SYNC_HI 0xA5
#define SERIAL_FRAME_SYNC_LO 0x5A

typedef struct SerialFrameHeader
{
    uint16_t sync; // SERIAL_FRAME_SYNC
    uint16_t len;  // payload length in bytes
} SerialFrameHeader;

#ifdef ARDUINO

#include <Stream.h>

// Send one payload with frame header over any Stream (HardwareSerial etc.).
inline void sendSerialFrame(Stream &port, const void *msg, uint16_t len)
{
    uint8_t header[4] = {
        SERIAL_FRAME_SYNC_HI,
        SERIAL_FRAME_SYNC_LO,
        (uint8_t)(len & 0xFF),
        (uint8_t)((len >> 8) & 0xFF)
    };
    port.write(header, sizeof(header));
    port.write((const uint8_t *)msg, len);
}

// Incremental frame parser: feed() one byte at a time from a UART RX loop.
// Returns the payload length when a complete frame just arrived (payload is
// then readable at payload()), otherwise 0. Corrupt frames (bogus length)
// resync on their own at the next frame header.
class SerialFrameReceiver
{
public:
    // Feed one received byte; returns payload length or 0.
    uint16_t feed(uint8_t b)
    {
        switch (state)
        {
        case ST_SYNC:
            if (prevWasA5 && b == SERIAL_FRAME_SYNC_LO)
                state = ST_LEN_LO;
            prevWasA5 = (b == SERIAL_FRAME_SYNC_HI);
            break;
        case ST_LEN_LO:
            frameLen = b;
            state = ST_LEN_HI;
            break;
        case ST_LEN_HI:
            frameLen |= ((uint16_t)b) << 8;
            if (frameLen == 0 || frameLen > MAX_PAYLOAD)
            {
                state = ST_SYNC; // bogus length, resync
            }
            else
            {
                idx = 0;
                state = ST_PAYLOAD;
            }
            break;
        case ST_PAYLOAD:
            buf[idx++] = b;
            if (idx >= frameLen)
            {
                state = ST_SYNC;
                prevWasA5 = false;
                return frameLen;
            }
            break;
        }
        return 0;
    }

    const uint8_t *payload() const { return buf; }

    static const uint16_t MAX_PAYLOAD = 64;

private:
    enum : uint8_t { ST_SYNC, ST_LEN_LO, ST_LEN_HI, ST_PAYLOAD };

    uint8_t  buf[MAX_PAYLOAD];
    uint16_t frameLen = 0;
    uint8_t  idx = 0;
    uint8_t  state = ST_SYNC;
    bool     prevWasA5 = false;
};

#endif // ARDUINO