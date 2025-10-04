#pragma once

#if defined(ARDUINO_ATTINY24_DIY)

// PS/2 interface configuration
#define PS2_CLOCK_PIN  10   /* PB0 */
#define PS2_DATA_PIN   9    /* PB1 */
#define PS2_RX_BUFFER_SIZE 16

// AppleII interface configuration
#define AII_RESET_PIN  8    /* PB2 */
#define AII_STROBE_PIN 0    /* PA0 */
#define AII_DATA_PIN_0 3    /* PA3 */
#define AII_DATA_PIN_1 1    /* PA1 */
#define AII_DATA_PIN_2 5    /* PA5 */
#define AII_DATA_PIN_3 4    /* PA4 */
#define AII_DATA_PIN_4 7    /* PA7 */
#define AII_DATA_PIN_5 2    /* PA2 */
#define AII_DATA_PIN_6 6    /* PA6 */

#else

// PS/2 interface configuration
#define PS2_CLOCK_PIN 7
#define PS2_DATA_PIN 8
#define PS2_RX_BUFFER_SIZE 16

// AppleII interface configuration
#define AII_RESET_PIN 0
#define AII_STROBE_PIN 1
#define AII_DATA_PIN_0 20
#define AII_DATA_PIN_1 21
#define AII_DATA_PIN_2 18
#define AII_DATA_PIN_3 19
#define AII_DATA_PIN_4 4
#define AII_DATA_PIN_5 3
#define AII_DATA_PIN_6 5

#endif