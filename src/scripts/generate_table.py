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
        if not plain_code:
            logger.info('PS/2 keycap [%s] does not map to an Apple II function', keycap)
            continue

        shifted_code = apple_keycodes.pop(secondary, plain_code)
        ctrled_code = apple_keycodes.pop('Ctrl+' + primary, plain_code)

        ctrl_shifted_code = apple_keycodes.pop('CtrlShift+' + primary, None)
        if not ctrl_shifted_code:
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


# Output a C++ lookup table that maps PS/2 scan code & modifier keys (shift, ctrl) to the
# Apple II keyboard code to emit.

print("// This table is generated from a script")
print("// DO NOT EDIT")
print("const uint8_t AppleIIKeyboardTranslation::translation[512][4] = {")
print("  // scan code, no modifier, SHIFT modifier, CTRL modifier, both modifiers")
for scancode in range(0, 0x200):
    scancode_key = "{0:02X}".format(scancode) if scancode < 0x100 \
        else "E0{0:02X}".format(scancode % 0x100)
    mapping = ps2_mapping.get(scancode_key, None)

    if mapping:
        print("  /* {:#05x} {:>8} */ {{{}, {}, {}, {}}},".format(
            scancode, '[' + mapping.keycap + ']', mapping.plain, mapping.shift,
            mapping.ctrl, mapping.ctrl_shift))
    else:
        print("  /* {:#05x} */ {{}},".format(scancode))

print("};")
