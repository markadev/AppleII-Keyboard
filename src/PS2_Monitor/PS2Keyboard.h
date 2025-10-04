#pragma once

#include <string.h>
#include "PS2Interface.h"


// Class to receive PS/2 keyboard scan codes from a PS/2 interface.
//
// The scan codes returned are slightly compressed to make table lookups easier. For scan
// codes with a leading E0 byte, it is replaced with a 0x01 and placed in the MSB of the
// uint16_t returned. So for example, the 'Delete' key with a scan code of E071 would return
// a code value of 0x0171.
class PS2Keyboard : private PS2Interface {
public:
    // Scan code values of some keys
    enum {
        SCAN_CODE_LSHIFT = 0x012,
        SCAN_CODE_RSHIFT = 0x059,
        SCAN_CODE_LCTRL = 0x014,
        SCAN_CODE_RCTRL = 0x114,
        SCAN_CODE_LALT = 0x011,
        SCAN_CODE_RALT = 0x111,
        SCAN_CODE_LWIN = 0x11f,
        SCAN_CODE_RWIN = 0x127,
        SCAN_CODE_MAX = 0x200,
    };

    PS2Keyboard();

    using PS2Interface::setup;
    using PS2Interface::onClockFallingEdge;

    // Returns the keypress received from the keyboard or 0 if there is none.
    uint16_t getKeypress();

    // Returns a boolean value indicating if a key with the given code is currently pressed.
    bool isKeyPressed(uint16_t code) const;

    // Returns a boolean value indicating if any shift key is pressed.
    bool isShiftPressed() const;

    // Returns a boolean value indicating if any ctrl key is pressed.
    bool isCtrlPressed() const;

    // Returns a boolean value indicating if any alt key is pressed.
    bool isAltPressed() const;

    // Returns a boolean value indicating if any Windows key is pressed.
    bool isWinPressed() const;

private:
    void updatePressedKeys(uint16_t code, bool isPressed);

    // bit definitions for m_pressedModifierKeys
    enum {
        PRESSED_LSHIFT = 0x01,
        PRESSED_RSHIFT = 0x02,
        PRESSED_LCTRL = 0x04,
        PRESSED_RCTRL = 0x08,
        PRESSED_LALT = 0x10,
        PRESSED_RALT = 0x20,
        PRESSED_LWIN = 0x40,
        PRESSED_RWIN = 0x80,
    };

    // bit definitions for m_flags
    enum {
        SCANCODE_BREAK = 0x01,      // next scancode is for a key-break (key being released)
        SCANCODE_EXTENDED = 0x02,   // next scancode is extended
    };

    uint8_t m_flags;
    uint8_t m_pauseBreakBytesToSkip;
    uint8_t m_pressedKeys[(SCAN_CODE_MAX+7) / 8];
};


PS2Keyboard::PS2Keyboard() : m_flags(0), m_pauseBreakBytesToSkip(0) {
    memset(m_pressedKeys, 0, sizeof(m_pressedKeys));
}


uint16_t PS2Keyboard::getKeypress() {
    uint16_t code;

    while(1) {
        uint8_t b = getDataByte();
        if(b == 0)
            break;

        if(m_pauseBreakBytesToSkip > 0) {
            m_pauseBreakBytesToSkip--;
            if(m_pauseBreakBytesToSkip == 0) {
                return 0x1ff;
            }
            continue;
        }

        switch(b) {
        case 0xf0:
            m_flags |= SCANCODE_BREAK;
            break;
        case 0xe0:
            m_flags |= SCANCODE_EXTENDED;
            break;
        case 0xe1:
            // skip over the mess that the pause/break key sends
            m_pauseBreakBytesToSkip = 7;
            break;
        default:
            // an actual scan code
            code = b;
            if(m_flags & SCANCODE_EXTENDED) {
                code |= 0x100;
            }

            if(!(m_flags & SCANCODE_BREAK)) {
                m_flags = 0;
                updatePressedKeys(code, true);
                return code;
            } else {
                m_flags = 0;
                updatePressedKeys(code, false);
            }
        }
    }

    return 0;
}


void PS2Keyboard::updatePressedKeys(uint16_t code, bool isPressed) {
    uint8_t byteIndex = (uint8_t)(code >> 3);
    if(byteIndex >= sizeof(m_pressedKeys))
        return;

    if(isPressed) {
        bitSet(m_pressedKeys[byteIndex], code & 0x07);
    } else {
        bitClear(m_pressedKeys[byteIndex], code & 0x07);
    }
}


bool PS2Keyboard::isKeyPressed(uint16_t code) const {
    uint8_t byteIndex = (uint8_t)(code >> 3);
    if(byteIndex >= sizeof(m_pressedKeys))
        return false;
    return bitRead(m_pressedKeys[byteIndex], code & 0x07);
}


bool PS2Keyboard::isShiftPressed() const {
    return isKeyPressed(SCAN_CODE_LSHIFT) || isKeyPressed(SCAN_CODE_RSHIFT);
}


bool PS2Keyboard::isCtrlPressed() const {
    return isKeyPressed(SCAN_CODE_LCTRL) || isKeyPressed(SCAN_CODE_RCTRL);
}


bool PS2Keyboard::isAltPressed() const {
    return isKeyPressed(SCAN_CODE_LALT) || isKeyPressed(SCAN_CODE_RALT);
}


bool PS2Keyboard::isWinPressed() const {
    return isKeyPressed(SCAN_CODE_LWIN) || isKeyPressed(SCAN_CODE_RWIN);
}
