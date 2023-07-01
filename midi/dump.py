import sys

from parser import *

def dump_file(buf):
    buf, header = parse_header(buf)
    print(header)
    assert header.ntrks > 0
    tracks = []
    for track_num in range(header.ntrks):
        buf, track = parse_track(buf)
        tracks.append(track)
        print(f"track {track_num}:")
        for i, event in enumerate(track.events):
            print('  ' + repr(event))

    print("remaining data:", len(buf))

import sys
with open(sys.argv[1], 'rb') as f:
    b = memoryview(f.read())

dump_file(b)
