#pragma once

#include "PS2Interface.h"


// Class to receive PS/2 keyboard scan codes from a PS/2 interface.
//
// The scan codes returned are slightly compressed to make table lookups easier. For scan
// codes with a leading E0 byte, it is replaced with a 0x01 and placed in the MSB of the
// uint16_t returned. So for example, the 'Delete' key with a scan code of E071 would return
// a code value of 0x0171.
class PS2Keyboard : private PS2Interface {
public:
  PS2Keyboard() : m_flags(0), m_pauseBreakBytesToSkip(0) {
    memset(m_pressedKeys, 0, sizeof(m_pressedKeys));
  }

  using PS2Interface::setup;
  using PS2Interface::onClockFallingEdge;

  // Returns the keypress received from the keyboard or 0 if there is none.
  uint16_t getKeypress();

  // Returns a boolean value indicating if a key with the given code is currently pressed.
  bool isKeyPressed(uint16_t code);

  // Returns a boolean value indicating if any shift key is pressed.
  bool isShiftPressed();

  // Returns a boolean value indicating if any ctrl key is pressed.
  bool isCtrlPressed();

  // Returns a boolean value indicating if any alt key is pressed.
  bool isAltPressed();

private:
  void updatePressedKeys(uint16_t code, bool isPressed);

  static const uint8_t BREAK = 0x01;
  static const uint8_t EXTENDED = 0x02;

  uint8_t m_flags;
  uint8_t m_pauseBreakBytesToSkip;
  uint8_t m_pressedKeys[512 / 8];
};

uint16_t PS2Keyboard::getKeypress() {
  for(uint8_t b = getDataByte(); b != 0; b = getDataByte()) {
    if(m_pauseBreakBytesToSkip > 0) {
      m_pauseBreakBytesToSkip--;
      if(m_pauseBreakBytesToSkip == 0) {
        return 0x1ff;
      }
      continue;
    }

    if(b == 0xf0) {
      m_flags |= BREAK;
    } else if(b == 0xe0) {
      m_flags |= EXTENDED;
    } else if(b == 0xe1) {
      // skip over the mess that the pause/break key sends
      m_pauseBreakBytesToSkip = 7;
    } else {
      // an actual scan code
      uint16_t code = b;
      if(m_flags & EXTENDED) {
        code |= 0x100;
      }

      if(!(m_flags & BREAK)) {
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
  if(isPressed) {
    bitSet(m_pressedKeys[byteIndex], code & 0x07);
  } else {
    bitClear(m_pressedKeys[byteIndex], code & 0x07);
  }
}

bool PS2Keyboard::isKeyPressed(uint16_t code) {
  uint8_t byteIndex = (uint8_t)(code >> 3);
  return bitRead(m_pressedKeys[byteIndex], code & 0x07); 
}

bool PS2Keyboard::isShiftPressed() {
  const uint16_t LSHIFT = 0x12;
  const uint16_t RSHIFT = 0x59;
  return isKeyPressed(LSHIFT) || isKeyPressed(RSHIFT);
}

bool PS2Keyboard::isCtrlPressed() {
  const uint16_t LCTRL = 0x14;
  const uint16_t RCTRL = 0x114;
  return isKeyPressed(LCTRL) || isKeyPressed(RCTRL);
}

bool PS2Keyboard::isAltPressed() {
  const uint16_t LALT = 0x11;
  const uint16_t RALT = 0x111;
  return isKeyPressed(LALT) || isKeyPressed(RALT);
}
