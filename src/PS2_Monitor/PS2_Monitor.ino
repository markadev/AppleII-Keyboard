// This sketch will receive codes from the PS2 keyboard and print them to the serial console

#include "Arduino.h"
#include "PS2Keyboard.h"
#include "config.h"


#if defined(ARDUINO_AVR_LEONARDO)
#  if ((PS2_CLOCK_PIN >= 8 && PS2_CLOCK_PIN <= 11) || (PS2_CLOCK_PIN >= 14 && PS2_CLOCK_PIN <= 17) || (PS2_CLOCK_PIN >= A8 && PS2_CLOCK_PIN <= A10))
#    define USE_INTERRUPT_PCINT0 1
#  endif
#elif defined(ARDUINO_ATTINY24_DIY)
#  if PS2_CLOCK_PIN < 8
#    define USE_INTERRUPT_PCINT0 1
#  else
#    define USE_INTERRUPT_PCINT1 1
#  endif
#endif


PS2Keyboard kbd;


#if USE_INTERRUPT_PCINT0

ISR(PCINT0_vect) {
    if(digitalRead(PS2_CLOCK_PIN) == 0)
        kbd.onClockFallingEdge();
}

#elif USE_INTERRUPT_PCINT1

ISR(PCINT1_vect) {
    if(digitalRead(PS2_CLOCK_PIN) == 0)
        kbd.onClockFallingEdge();
}

#else

void clockEdgeIntHandler() {
    kbd.onClockFallingEdge();
}

#endif


void setup() {
#ifndef ARDUINO_ATTINY24_DIY
    Serial.begin(115200);
#endif

    kbd.setup();

#if USE_INTERRUPT_PCINT0 || USE_INTERRUPT_PCINT1
    if(digitalPinToPCICR(PS2_CLOCK_PIN)) {
        // enable interrupt when the pin state changes
        *digitalPinToPCICR(PS2_CLOCK_PIN) |= (1 << digitalPinToPCICRbit(PS2_CLOCK_PIN));
        *digitalPinToPCMSK(PS2_CLOCK_PIN) |= (1 << digitalPinToPCMSKbit(PS2_CLOCK_PIN));
    } else
#else
    if(digitalPinToInterrupt(PS2_CLOCK_PIN) != NOT_AN_INTERRUPT) {
        // attach external interrupt on pin's falling edge
        attachInterrupt(digitalPinToInterrupt(PS2_CLOCK_PIN), clockEdgeIntHandler, FALLING);
    } else
#endif
    {
        // Cause a linking failure if the PS2_CLOCK_PIN doesn't support the interrupts.
        // This is preferable to failing silently at runtime.
        extern void invalid_PS2_CLOCK_PIN();
        invalid_PS2_CLOCK_PIN();
    }
}


void loop() {
    uint16_t d = kbd.getKeypress();
    if(d != 0) {
#ifndef ARDUINO_ATTINY24_DIY
        Serial.print(d, HEX);
        Serial.write('\n');
#endif
    }
}
