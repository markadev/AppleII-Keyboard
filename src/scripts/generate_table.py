#!/usr/bin/env python3

import collections
import json
import itertools
import logging
import sys


logging.basicConfig(level=logging.DEBUG, format='[%(levelname)s] %(message)s')

logger = logging.getLogger(__name__)


AppleKeyMapping = collections.namedtuple('AppleKeyMapping',
    ['plain', 'ctrl', 'shift', 'ctrl_shift', 'keycap'])


# Create a map of relevant Apple II key combos to Apple II keyboard codes
apple_keycodes = {}
with open('appleii_asciicodes.txt') as fp:
    for line in fp:
        line = line.rstrip()
        keycap, plain_code, ctrled_code, shifted_code, ctrl_shifted_code = line.split(' ')
        plain_code = int(plain_code, 0)
        ctrled_code = int(ctrled_code, 0)
        shifted_code = int(shifted_code, 0)
        ctrl_shifted_code = int(ctrl_shifted_code, 0)

        if shifted_code != plain_code:
            assert len(keycap) == 2, "Expected two chars on the keycap [{}]".format(keycap)

            primary, secondary = keycap[0], keycap[1]
            apple_keycodes[primary] = plain_code
            apple_keycodes[secondary] = shifted_code

            if ctrled_code != plain_code:
                logger.debug("Ctrl modifier relevant for Apple II keycap [%s]", keycap)
                apple_keycodes['Ctrl+' + primary] = ctrled_code

            if ctrl_shifted_code != shifted_code:
                logger.debug("CtrlShift modifier relevant for Apple II keycap [%s]", keycap)
                apple_keycodes['CtrlShift+' + primary] = ctrl_shifted_code
        else:
            assert len(keycap) != 2, "Expected one char on the keycap [{}]".format(keycap)
            assert shifted_code == plain_code, "Expected no shift modifier effect"
            assert ctrl_shifted_code == ctrled_code, "Expected no shift modifier effect"

            apple_keycodes[keycap] = plain_code
            if ctrled_code != plain_code:
                logger.debug("Ctrl modifier relevant for Apple II keycap [%s]", keycap)
                apple_keycodes['Ctrl+' + keycap] = ctrled_code


# For each PS/2 keyboard key, try to match it to a corresponding Apple II keyboard function
ps2_mapping = {}
with open('ps2_scancodes.txt') as fp:
    for line in fp:
        line = line.rstrip()
        scancode, keycap = line.split(' ')

        primary = keycap
        secondary = None
        if len(keycap) == 2:
            primary, secondary = keycap[0], keycap[1]

        plain_code = apple_keycodes.pop(primary, None)
        if plain_code is None:
            logger.info('PS/2 keycap [%s] does not map to an Apple II function', keycap)
            continue

        shifted_code = apple_keycodes.pop(secondary, plain_code)
        ctrled_code = apple_keycodes.pop('Ctrl+' + primary, plain_code)

        ctrl_shifted_code = apple_keycodes.pop('CtrlShift+' + primary, None)
        if ctrl_shifted_code is None:
            if shifted_code != plain_code:
                # This is primarily a shifted key
                ctrl_shifted_code = shifted_code
            elif ctrled_code != plain_code:
                # This is primarily a ctrl'd key
                ctrl_shifted_code = ctrled_code
            else:
                ctrl_shifted_code = plain_code

        ps2_mapping[scancode] = AppleKeyMapping(plain_code, ctrled_code, shifted_code, ctrl_shifted_code, keycap)


# Check that all Apple II keys are representated in the mapping
if len(apple_keycodes) != 0:
    logger.warning("Some Apple II keyboard keys are not represented:")
    for k in apple_keycodes:
        logger.warning("  - [%s]", k)
    sys.exit(1)


#print(json.dumps(ps2_mapping, indent='  '))

def scancode_key(scancode):
    return "{0:02X}".format(scancode) if scancode < 0x100 \
        else "E0{0:02X}".format(scancode % 0x100)

# build a single lookup table of scancode to ASCII codes
scancode_table = [ ps2_mapping.get(scancode_key(scancode), None) for scancode in range(512) ]


# Split the table into two, removing the largest range of scan codes with no mapping
def split_scancode_table(scancode_table):
    # First remove empty mappings from the end of the table
    while scancode_table and scancode_table[-1] is None:
        scancode_table = scancode_table[:-1]

    # Find the first non-empty mapping
    span1_start = 0
    for i in range(len(scancode_table)):
        if scancode_table[i] is not None:
            span1_start = i
            break

    # Find the largest span of empty mappings in the list to cut out
    range_start, range_size = 0, 0
    cut_start, cut_size = 0, 0
    for i in range(len(scancode_table)):
        if scancode_table[i] is None:
            if range_start + range_size == i:
                # empty mapping is part of the current span
                range_size += 1
            else:
                # start of a new empty mapping span
                range_start, range_size = i, 1
        else:
            # found the end of the current empty mapping span
            if range_size > cut_size:
                cut_start, cut_size = range_start, range_size

    # Cut out the range of empty mappings, returning the spans before & after it
    return span1_start, scancode_table[span1_start:cut_start], \
        cut_start + cut_size, scancode_table[cut_start + cut_size:]


span1_start, span1, span2_start, span2 = split_scancode_table(scancode_table)


# Output a C++ class that maps PS/2 scan code & modifier keys (shift, ctrl) to the
# Apple II keyboard code to emit.
print(f"""#pragma once
#include "PS2Keyboard.h"

// This file is generated from a script
// DO NOT EDIT

// Class to translate PS2 scan codes into Apple II keyboard codes.
class AppleIIKeyboardTranslation {{
public:
    AppleIIKeyboardTranslation(PS2Keyboard& kbd) : m_kbd(kbd) {{}}

    // Fetches the next encoded key code, or 0 if there isn't one.
    uint8_t getKeyCode();

private:
    PS2Keyboard& m_kbd;

    enum {{
        SCANCODE_SPAN1_START = {span1_start},
        SCANCODE_SPAN1_SIZE = {len(span1)},
        SCANCODE_SPAN2_START = {span2_start},
        SCANCODE_SPAN2_SIZE = {len(span2)},
    }};
    static const uint8_t scancode_span1[SCANCODE_SPAN1_SIZE][4] PROGMEM;
    static const uint8_t scancode_span2[SCANCODE_SPAN2_SIZE][4] PROGMEM;
}};
""")

print("""
uint8_t AppleIIKeyboardTranslation::getKeyCode() {
    uint16_t scanCode = m_kbd.getKeypress();

    uint8_t modifiers = 0;
    if(m_kbd.isShiftPressed()) {
        modifiers |= 0x1;
    }
    if(m_kbd.isCtrlPressed()) {
        modifiers |= 0x2;
    }

    if(scanCode < SCANCODE_SPAN1_START) {
        return 0;
    } else if(scanCode < SCANCODE_SPAN1_START+SCANCODE_SPAN1_SIZE) {
        return pgm_read_byte(&(scancode_span1[scanCode - SCANCODE_SPAN1_START][modifiers]));
    }

    if(scanCode < SCANCODE_SPAN2_START) {
        return 0;
    } else if(scanCode < SCANCODE_SPAN2_START+SCANCODE_SPAN2_SIZE) {
        return pgm_read_byte(&(scancode_span2[scanCode - SCANCODE_SPAN2_START][modifiers]));
    }
    return 0;
}
""")


print("const uint8_t AppleIIKeyboardTranslation::scancode_span1[AppleIIKeyboardTranslation::SCANCODE_SPAN1_SIZE][4] = {")
print("    // scan code, no modifier, SHIFT modifier, CTRL modifier, both modifiers")
for scancode, mapping in ((span1_start+i, span1[i]) for i in range(len(span1))):
    if mapping is not None:
        print("    /* {:#05x} {:>8} */ {{{:#04x}, {:#04x}, {:#04x}, {:#04x}}},".format(
            scancode, '[' + mapping.keycap + ']', mapping.plain, mapping.shift,
            mapping.ctrl, mapping.ctrl_shift))
    else:
        print("    /* {:#05x} */ {{}},".format(scancode))
print("};\n")


print("const uint8_t AppleIIKeyboardTranslation::scancode_span2[AppleIIKeyboardTranslation::SCANCODE_SPAN2_SIZE][4] = {")
print("    // scan code, no modifier, SHIFT modifier, CTRL modifier, both modifiers")
for scancode, mapping in ((span2_start+i, span2[i]) for i in range(len(span2))):
    if mapping is not None:
        print("    /* {:#05x} {:>8} */ {{{:#04x}, {:#04x}, {:#04x}, {:#04x}}},".format(
            scancode, '[' + mapping.keycap + ']', mapping.plain, mapping.shift,
            mapping.ctrl, mapping.ctrl_shift))
    else:
        print("    /* {:#05x} */ {{}},".format(scancode))
print("};")
