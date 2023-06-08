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
            k, v1, v2 = line.strip().split()
            yield k, (v1, v2)


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
printable["SPACE"] = (' ', ' ')

def e(s):
    if s == '\\':
        return '\\\\'
    elif s == '\'':
        return '\\\''
    else:
        return s

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
    print("int32_t keysym_to_char(enum keysym k, bool shift);")

    
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


    print("static constexpr inline int32_t unshift_char(enum keysym k)")
    print("{")
    print("  switch(k) {")
    for keysym, _ in keymap:
        if keysym in printable:
            value = printable[keysym]
            print(f"  case keysym::{keysym}: return '{e(value[0])}';")
    print("  default: return -1;")
    print("  }")
    print("}")

    print()

    print("static constexpr inline int32_t shift_char(enum keysym k)")
    print("{")
    print("  switch(k) {")
    for keysym, _ in keymap:
        if keysym in printable:
            value = printable[keysym]
            print(f"  case keysym::{keysym}: return '{e(value[1])}';")
    print("  default: return -1;")
    print("  }")
    print("}")

    print()
    
    print("""int32_t keysym_to_char(enum keysym k, bool shift)
{
  if (shift) {
    return shift_char(k);
  } else {
    return unshift_char(k);
  }
}
""")
