import struct
import sys

input_file = sys.argv[1]
output_file = sys.argv[2]

with open(input_file, 'r') as f:
    buf = f.read()

def parse_input(buf):
    lines = buf.strip().split('\n')
    for line in lines:
        if not line.strip():
            yield 0
        else:
            yield int(line)

with open(output_file, 'wb') as f:
    for num in parse_input(buf):
        f.write(struct.pack(">i", num))
