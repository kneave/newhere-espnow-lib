// Teensy 4.1 <-> Adafruit QT Py ESP32-S2 serial link
//
// Example for the newhere-espnow-lib library.
//
// Roles:
//   - RECEIVES: Joystick_Msg / Position_Msg forwarded by the ESP32
//     (relayed from ESP-NOW), framed as [0xA5][0x5A][len_lo][len_hi][payload]
//   - SENDS: Power_Msg battery readings at 1 Hz, same framing
//
// The ESP32 beacon side lives in the beacon project: it receives ESP-NOW,
// forwards Joystick_Msg/Position_Msg out over Serial1 using sendSerialFrame(),
// and polls for Power_Msg with a SerialFrameReceiver.
//
// Wiring (Teensy 4.1 -> QT Py ESP32-S2):
//   Serial1 TX1 (pin 1)  -> QT Py RX  (GPIO16)
//   Serial1 RX1 (pin 0)  -> QT Py TX  (GPIO5)
//   GND                  -> GND
// Both boards are 3.3V logic - no level shifting needed.

#include <newhere-espnow-lib.h>

Joystick_Msg  joy_msg;
Position_Msg  position_msg;
Power_Msg     power_msg;

SerialFrameReceiver esp32Rx;

unsigned long lastPowerSend = 0;
#define POWER_INTERVAL_MS 1000

void setup() {
  Serial.begin(115200);   // USB host console
  Serial1.begin(115200);  // Teensy pins 0/1 - to the QT Py
  delay(500);

  Serial.println("Teensy ready: receiving joy/pos from ESP32, sending power");
}

void loop() {
  while (Serial1.available()) {
    uint16_t len = esp32Rx.feed(Serial1.read());
    if (len == 0) {
      continue;
    }

    if (len == sizeof(Joystick_Msg)) {
      memcpy(&joy_msg, esp32Rx.payload(), sizeof(Joystick_Msg));
      // Replace with real consumer (motor drive, UI, etc.)
      Serial.print("joy: x=");
      Serial.print(joy_msg.x, 3);
      Serial.print(" y=");
      Serial.print(joy_msg.y, 3);
      Serial.print(" c=");
      Serial.print(joy_msg.c);
      Serial.print(" z=");
      Serial.print(joy_msg.z);
      Serial.print(" t=");
      Serial.println(joy_msg.timestamp);
    } else if (len == sizeof(Position_Msg)) {
      memcpy(&position_msg, esp32Rx.payload(), sizeof(Position_Msg));
      Serial.print("pos: lat=");
      Serial.print(position_msg.lat, 6);
      Serial.print(" lon=");
      Serial.print(position_msg.lon, 6);
      Serial.print(" rtk=");
      Serial.println(position_msg.rtk);
    }
  }

  unsigned long now = millis();
  if (now - lastPowerSend >= POWER_INTERVAL_MS) {
    lastPowerSend = now;

    // Demo battery data - replace with real ADC/coulomb-counter readings
    power_msg.voltage = 3.7f + 0.1f * sinf(now * 0.001f);
    power_msg.current = 0.5f + 0.1f * cosf(now * 0.0007f);
    sendSerialFrame(Serial1, &power_msg, sizeof(power_msg));

    Serial.print("sent power: v=");
    Serial.print(power_msg.voltage, 3);
    Serial.print(" i=");
    Serial.println(power_msg.current, 3);
  }
}