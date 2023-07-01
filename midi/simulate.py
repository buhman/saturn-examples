from itertools import chain
from parser import *

"""
_slots = set()

def simulate_note(ix, ev):
    if type(ev.event) is NoteOn:
        print(repr(ev.event))

        _slots.add((ev.event.channel, ev.event.note))
        assert len(_slots) <= 32, (hex(ix))
    if type(ev.event) is NoteOff:
        print(repr(ev.event))
        try:
            _slots.remove((ev.event.channel, ev.event.note))
        except:
            print("ix", hex(ix))
            raise
"""

def linearize_track(track):
    time = 0
    for i, event in enumerate(track.events):
        time += event.delta_time
        yield time, i, event

l = {
    NoteOff: 0,
    NoteOn: 1,
    PolyphonicKeyPressure: 2,
    ControlChange: 3,
    ProgramChange: 4,
    ChannelPressure: 5,
    PitchBendChange: 6,
    ChannelMode: 7,
}

def sort_key(global_time, i, event):
    channel = event.event.event.channel if type(event.event) is MIDIEvent else 99
    priority = -l[type(event.event.event)] if type(event.event) is MIDIEvent else -99
    return global_time, priority, channel

def linearize_events(tracks):
    global_time = 0
    linearized = list(chain.from_iterable(map(linearize_track, tracks)))
    linear_sort = sorted(linearized, key=lambda args: sort_key(*args))
    for abs_time, i, event in linear_sort:
        if type(event.event) is MetaEvent and event.event.type is MetaType.EndOfTrack:
            continue
        inner_event = event.event
        delta_time = abs_time - global_time
        print(Event(delta_time, inner_event))
        global_time = abs_time
    print(Event(delta_time=0, event=MetaEvent(type=MetaType.EndOfTrack, value=None)))

def dump_file(buf):
    buf, header = parse_header(buf)
    print(header)
    assert header.ntrks > 0
    tracks = []
    for track_num in range(header.ntrks):
        buf, track = parse_track(buf)
        tracks.append(track)
    print("remaining data:", len(buf))
    assert len(buf) == 0, bytes(buf)

    linearize_events(tracks)

import sys
with open(sys.argv[1], 'rb') as f:
    b = memoryview(f.read())

dump_file(b)
