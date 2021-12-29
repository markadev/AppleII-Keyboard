#pragma once

#include "config.h"


class Latch7 {
public:
  void setup() {
    digitalWrite(AII_STROBE_PIN, 0); // strobe is active-high
    pinMode(AII_STROBE_PIN, OUTPUT);
    pinMode(AII_DATA_PIN_0, OUTPUT);
    pinMode(AII_DATA_PIN_1, OUTPUT);
    pinMode(AII_DATA_PIN_2, OUTPUT);
    pinMode(AII_DATA_PIN_3, OUTPUT);
    pinMode(AII_DATA_PIN_4, OUTPUT);
    pinMode(AII_DATA_PIN_5, OUTPUT);
    pinMode(AII_DATA_PIN_6, OUTPUT);
  }

  void write(uint8_t value) {
    digitalWrite(AII_DATA_PIN_0, value & 0x01);
    digitalWrite(AII_DATA_PIN_1, (value >> 1) & 0x01);
    digitalWrite(AII_DATA_PIN_2, (value >> 2) & 0x01);
    digitalWrite(AII_DATA_PIN_3, (value >> 3) & 0x01);
    digitalWrite(AII_DATA_PIN_4, (value >> 4) & 0x01);
    digitalWrite(AII_DATA_PIN_5, (value >> 5) & 0x01);
    digitalWrite(AII_DATA_PIN_6, (value >> 6) & 0x01);

    // Ref p.90 of "The Apple II Circuit Description" for timing info
    delayMicroseconds(11);
    digitalWrite(AII_STROBE_PIN, 1);
    delayMicroseconds(11);
    digitalWrite(AII_STROBE_PIN, 0);
    delayMicroseconds(11);
  }

private:
  int m_clkDelay;
};
