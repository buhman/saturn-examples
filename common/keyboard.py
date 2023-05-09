from operator import itemgetter

import os

abspath = os.path.abspath(__file__)
dirname = os.path.dirname(abspath)
os.chdir(dirname)


def parse_input():
    with open("keyboard.txt", "r") as f:
        for line in f:
            if not line.strip():
                continue
            yield line.strip().split()


def parse_printable():
    with open("printable.txt", "r") as f:
        for line in f:
            if not line.strip():
                continue
            yield line.strip().split()


scancodes = set()
keysyms = set()


def build_keymap():
    _keymap = []
    global keysyms
    global scancodes

    for keysym, _scancode in parse_input():
        scancode = int(_scancode, 16)
        assert keysym not in keysyms, keysym
        assert scancode not in scancodes, hex(scancode)
        keysyms.add(keysym)
        scancodes.add(scancode)
        _keymap.append((keysym, scancode))

    return sorted(_keymap, key=itemgetter(1)) # sort by scancode, ascending

keymap = build_keymap()


printable = dict(parse_printable())
printable["SPACE"] = ' '

import sys
if sys.argv[1] == "header":
    print("#include <stdint.h>")
    print()

    print("enum struct keysym {")
    print(f"  UNKNOWN,")
    for keysym, _ in keymap:
        print(f"  {keysym},")
    print("};")

    print()

    print("enum keysym scancode_to_keysym(uint32_t scancode);")
    print("char16_t keysym_to_char16(enum keysym k);")

if sys.argv[1] == "definition":
    print("#include <stdint.h>")
    print("#include \"keyboard.hpp\"")

    print("enum keysym scancode_to_keysym(uint32_t scancode)")
    print("{")
    print("  switch(scancode) {")
    for keysym, scancode in keymap:
        print(f"  case 0x{scancode:02x}: return keysym::{keysym};")
    print("  default: return keysym::UNKNOWN;")
    print("  }")
    print("}")


    print()


    print("char16_t keysym_to_char16(enum keysym k)")
    print("{")
    print("  switch(k) {")
    for keysym, _ in keymap:
        if keysym in printable:
            value = printable[keysym]
            if value == '\\':
                value = '\\\\';
            print(f"  case keysym::{keysym}: return '{value}';")
    print("  default: return -1;")
    print("  }")
    print("}")
