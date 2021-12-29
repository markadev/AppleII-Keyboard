#include "Arduino.h"
#include "PS2Keyboard.h"
#include "AppleIIKeyboardTranslation.h"
#include "Latch7.h"

#define ENABLE_SERIAL_DEBUG 0


PS2Keyboard kbd;
AppleIIKeyboardTranslation appleKbd(kbd);
Latch7 appleOutputLatch;

void ps2ClockIntHandler() {
  kbd.onClockFallingEdge();
}

void setup() {
#if ENABLE_SERIAL_DEBUG
  Serial.begin(115200);
#endif

  appleOutputLatch.setup();

  digitalWrite(AII_RESET_PIN, 1); // reset is active-low
  pinMode(AII_RESET_PIN, OUTPUT);

  kbd.setup();
  attachInterrupt(digitalPinToInterrupt(PS2_CLOCK_PIN), ps2ClockIntHandler, FALLING);
}

void loop() {
  uint8_t d = appleKbd.getKeyCode();
  if(d != 0) {
#if ENABLE_SERIAL_DEBUG
    Serial.print(d, HEX);
    Serial.write('\n');
#endif

    if(d == 0xff) {
      // Special reset key sequence was pressed
      digitalWrite(AII_RESET_PIN, 0);
      delay(50);
      digitalWrite(AII_RESET_PIN, 1);
    } else {
      appleOutputLatch.write(d);
    }
  }
}
