#include "Arduino.h"
#include "PS2Keyboard.h"
#include "AppleIIKeyboardTranslation.h"
#include "Latch7.h"
#include "config.h"


// Arduino IDE settings for Arduino Pro Micro clone:
//  1. Under Tools->Board, select the Arduino Leonardo type from the Arduino AVR Boards
//
// Arduino IDE board settings for ATtiny
//  1. Install the DIY-ATtiny board package (https://raw.githubusercontent.com/sleemanj/optiboot/master/dists/package_gogo_diy_attiny_index.json)
//  2. Under Tools->Board, select the ATtiny24 type from the DIY-ATtiny board type
//  3. Under Tools->Processor Version, select ATtiny24a
//  4. Under Tools->Use Bootloader, select No
//  5. Under Tools->Override Clock Source, select Internal Oscillator 8MHz


#define ENABLE_SERIAL_DEBUG 0

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
AppleIIKeyboardTranslation appleKbd(kbd);
Latch7 appleOutputLatch;


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
#if ENABLE_SERIAL_DEBUG
    Serial.begin(115200);
#endif

    appleOutputLatch.setup();

    // leave AII_RESET_PIN configured as an INPUT so it floats
    digitalWrite(AII_RESET_PIN, 0); // drive low when the pin is set to output

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
    uint8_t d = appleKbd.getKeyCode();
    if(d != 0) {
#if ENABLE_SERIAL_DEBUG
        Serial.print(d, HEX);
        Serial.write('\n');
#endif

        if(d == 0xff) {
            // Special reset key sequence was pressed
            pinMode(AII_RESET_PIN, OUTPUT);  // drive low
            delay(100);
            pinMode(AII_RESET_PIN, INPUT);  // back to floating
        } else {
            appleOutputLatch.write(d);
        }
    }
}
