// This sketch will receive codes from the PS2 keyboard and print them to the serial console

#include "Arduino.h"
#include "PS2Keyboard.h"
#include "config.h"


PS2Keyboard kbd;


void ps2ClockIntHandler() {
    kbd.onClockFallingEdge();
}


void setup() {
    Serial.begin(115200);

    kbd.setup();
    attachInterrupt(digitalPinToInterrupt(PS2_CLOCK_PIN), ps2ClockIntHandler, FALLING);
}


void loop() {
    uint16_t d = kbd.getKeypress();
    if(d != 0) {
        Serial.print(d, HEX);
        Serial.write('\n');
    }
}
