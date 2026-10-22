# newhere-espnow-lib

Shared message definitions and framed-UART transport for the NewHere beacon
link between an ESP32 (Adafruit QT Py ESP32-S2) and a Teensy 4.1.

## Message structs

| Struct | Size | Carries |
|---|---|---|
| `Joystick_Msg` | 20 B | x, y, c, z, timestamp |
| `Power_Msg` | 8 B | voltage, current |
| `Position_Msg` | 24 B | lat, lon, alt, acc (mm), rtk solution, unixtime |

All members use fixed-width types (`int32_t`/`uint32_t`/`float`), so the byte
layout is identical on ESP32 and Teensy 4.1: both are little-endian ARM/Xtensa
with IEEE-754 floats and no struct padding.

## Transports

**ESP-NOW** — send the struct bytes directly (packet boundaries are native):

```cpp
esp_now_send(peer, (uint8_t *)&position_msg, sizeof(position_msg));
```

**UART (ESP32 <-> Teensy)** — prefix each payload with a 4-byte frame header
`[0xA5][0x5A][len_lo][len_hi]` so a stream receiver can find boundaries.
Use the library helpers on both ends:

```cpp
#include <newhere-espnow-lib.h>

SerialFrameReceiver rx;   // one instance per UART

void loop() {
  // sending (either side)
  sendSerialFrame(Serial1, &power_msg, sizeof(power_msg));

  // receiving (either side)
  while (Serial1.available()) {
    uint16_t len = rx.feed(Serial1.read());
    if (len == sizeof(Power_Msg)) {
      memcpy(&power_msg, rx.payload(), sizeof(Power_Msg));
    }
  }
}
```

Receiver state machine: hunt for the 2-byte sync, validate length (max 64 B),
collect payload, then dispatch by length as above. A corrupt or shifted
stream resyncs automatically at the next frame header.

## Message flow

```
+-----------+ ESP-NOW +-----------+ UART 0xA5|0x5A +-----------+
|遥控 sender | -------> |  Beacon   | -------------> |  Teensy   |
| (joy/pos) |          | (QT Py S2)|  <------------- |  4.1     |
+-----------+          +-----------+   Power_Msg    +-----------+
```

## Example

[examples/TeensySerialLink/TeensySerialLink.ino](examples/TeensySerialLink/TeensySerialLink.ino)
is the Teensy 4.1 side: receives forwarded joystick/position frames from the
beacon over Serial1 (pins 0/1), and sends Power_Msg battery readings at 1 Hz.

### Wiring (Teensy 4.1 -> QT Py ESP32-S2)

| Teensy | QT Py |
|---|---|
| TX1 (pin 1) | RX (GPIO16) |
| RX1 (pin 0) | TX (GPIO5) |
| GND | GND |

Both boards are 3.3 V logic — connect directly.

## Adding a message type

1. Add the struct here (fixed-width types only, one natural alignment — no
   `#pragma pack`).
2. Bump nothing else: dispatch is by payload length at runtime.
3. Update the ESP-NOW sender, the beacon's `OnDataRecv` / `pollTeensySerial`
   dispatch, and the Teensy example.

## License

MIT