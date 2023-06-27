import struct
from dataclasses import dataclass
from typing import *
import enum

def parse_variable_length(buf):
    n = 0
    i = 0
    while i < 4:
        n <<= 7
        b = buf[i]
        i += 1
        n |= (b & 0x7f)
        if not b & 0x80:
            break
    else:
        assert False, bytes(buf[0:5])
    return buf[i:], n

def parse_header_chunk_type(buf):
    assert buf[0] == ord('M'), bytes(buf[0:4])
    assert buf[1] == ord('T'), bytes(buf[0:4])
    assert buf[2] == ord('h'), bytes(buf[0:4])
    assert buf[3] == ord('d'), bytes(buf[0:4])
    return buf[4:], None

def parse_uint32(buf):
    n, = struct.unpack('>L', buf[:4])
    return buf[4:], n

def parse_uint16(buf):
    n, = struct.unpack('>H', buf[:2])
    return buf[2:], n

@dataclass
class MetricalDivision:
    ticks_per_quarter_note: int

@dataclass
class TimeCodeDivision:
    smpte: int
    ticks_per_frame: int

@dataclass
class Header:
    format: int
    ntrks: int
    division: Union[MetricalDivision, TimeCodeDivision]

def parse_division(buf):
    buf, n = parse_uint16(buf)
    if n & (1 << 15):
        smpte = ((n >> 8) & 0x7f) - 0x80 # sign-extend
        ticks_per_frame = n & 0xff
        division = TimeCodeDivision(smpte, ticks_per_frame)
    else:
        ticks_per_quarter_note = n
        division = MetricalDivision(ticks_per_quarter_note)
    return buf, division

    
def parse_header(buf):
    buf, _ = parse_header_chunk_type(buf)
    buf, length = parse_uint32(buf)
    assert length == 6, length
    buf, format = parse_uint16(buf)
    assert format in {0, 1, 2}, format
    buf, ntrks = parse_uint16(buf)
    buf, division = parse_division(buf)

    return buf, Header(format, ntrks, division)

@dataclass
class NoteOff:
    channel: int
    note: int
    velocity: int

@dataclass
class NoteOn:
    channel: int
    note: int
    velocity: int

@dataclass
class PolyphonicKeyPressure:
    channel: int
    note: int
    pressure: int


control_change_description = dict([
    (0x00, "Bank Select"),
    (0x01, "Modulation wheel or lever"),
    (0x02, "Breath Controller"),
    (0x04, "Foot controller"),
    (0x05, "Portamento time"),
    (0x06, "Data entry MSB"),
    (0x07, "Channel Volume"),
    (0x08, "Balance"),
    (0x0A, "Pan"),
    (0x0B, "Expression Controller"),
    (0x0C, "Effect Control 1"),
    (0x0D, "Effect Control 2"),
    (0x10, "General Purpose Controller 1"),
    (0x11, "General Purpose Controller 2"),
    (0x12, "General Purpose Controller 3"),
    (0x13, "General Purpose Controller 4"),
    (0x40, "Damper pedal (sustain)"),
    (0x41, "Portamento On/Off"),
    (0x42, "Sostenuto"),
    (0x43, "Soft pedal"),
    (0x44, "Legato Footswitch (vv = 00-3F:Normal, 40-7F=Legatto)"),
    (0x45, "Hold 2"),
    (0x46, "Sound Controller 1 (default: Sound Variation)"),
    (0x47, "Sound Controller 2 (default: Timbre/Harmonic Intensity)"),
    (0x48, "Sound Controller 3 (default: Release Time)"),
    (0x49, "Sound Controller 4 (default: Attack Time)"),
    (0x4A, "Sound Controller 5 (default: Brightness)"),
    (0x4B, "Sound Controller 6 (default: no default)"),
    (0x4C, "Sound Controller 7 (default: no default)"),
    (0x4D, "Sound Controller 8 (default: no default)"),
    (0x4E, "Sound Controller 9 (default: no default)"),
    (0x4F, "Sound Controller 10 (default: no default)"),
    (0x50, "General Purpose Controller 5"),
    (0x51, "General Purpose Controller 6"),
    (0x52, "General Purpose Controller 7"),
    (0x53, "General Purpose Controller 9"),
    (0x54, "Portamento Control"),
    (0x5B, "Effects 1 Depth"),
    (0x5C, "Effects 2 Depth"),
    (0x5D, "Effects 3 Depth"),
    (0x5E, "Effects 4 Depth"),
    (0x5F, "Effects 5 Depth"),
    (0x60, "Data increment"),
    (0x61, "Data decrement"),
    (0x62, "Non-Registered Parameter Number LSB"),
    (0x63, "Non-Registered Parameter Number MSB"),
    (0x64, "Registered Parameter Number LSB"),
    (0x65, "Registered Parameter Number MSB"),
])

@dataclass
class ControlChange:
    channel: int
    control: int
    value: int

    def __init__(self, channel, control, value):
        self.channel = channel
        self.control = (control, control_change_description.get(control, "Undefined"))
        self.value = value

@dataclass
class ProgramChange:
    channel: int
    program: int

@dataclass
class ChannelPressure:
    channel: int
    pressure: int

@dataclass
class PitchBendChange:
    channel: int
    lsb: int
    msb: int

@dataclass
class ChannelMode:
    channel: int
    controller: int
    value: int

@dataclass
class MIDIEvent:
    event = Union[
        NoteOff,
        NoteOn,
        PolyphonicKeyPressure,
        ControlChange,
        ProgramChange,
        ChannelPressure,
        PitchBendChange,
        ChannelMode,
    ]

@dataclass
class SysexEvent:
    data: bytes

class MetaType(enum.Enum):
    SequenceNumber = enum.auto()
    TextEvent = enum.auto()
    CopyrightNotice = enum.auto()
    SequenceTrackName = enum.auto()
    InstrumentName = enum.auto()
    Lyric = enum.auto()
    Marker = enum.auto()
    CuePoint = enum.auto()
    MIDIChannelPrefix = enum.auto()
    EndOfTrack = enum.auto()
    SetTempo = enum.auto()
    SMPTEOffset = enum.auto()
    TimeSignature = enum.auto()
    KeySignature = enum.auto()
    SequencerSpecific = enum.auto()

    def __repr__(self):
        return self.name

def _sequence_number(b):
    assert len(b) == 2, b
    n, = struct.unpack('<H', b[:2])
    return n

def _midi_channel_prefix(b):
    assert len(b) == 1, b
    return b[0]

def _set_tempo(b):
    assert len(b) == 3, b
    n, = struct.unpack('<L', bytes([0, *b[:3]]))
    return n

@dataclass
class SMPTEOffsetValue:
    hr: int
    mn: int
    se: int
    fr: int
    ff: int

def _smpte_offset(b):
    assert len(b) == 5, b
    return SMPTEOffsetValue(*b)

@dataclass
class TimeSignature:
    nn: int
    dd: int
    cc: int
    bb: int

def _time_signature(b):
    assert len(b) == 4, b
    return TimeSignature(*b)

@dataclass
class KeySignature:
    sf: int
    mi: int

def _key_signature(b):
    assert len(b) == 2
    return KeySignature(*b)

def _bytes(b):
    return bytes(b)

def _none(b):
    return None

meta_nums = dict([
    (0x00, (MetaType.SequenceNumber, _sequence_number)),
    (0x01, (MetaType.TextEvent, _bytes)),
    (0x02, (MetaType.CopyrightNotice, _bytes)),
    (0x03, (MetaType.SequenceTrackName, _bytes)),
    (0x04, (MetaType.InstrumentName, _bytes)),
    (0x05, (MetaType.Lyric, _bytes)),
    (0x06, (MetaType.Marker, _bytes)),
    (0x07, (MetaType.CuePoint, _bytes)),
    (0x20, (MetaType.MIDIChannelPrefix, _midi_channel_prefix)),
    (0x2f, (MetaType.EndOfTrack, _none)),
    (0x51, (MetaType.SetTempo, _set_tempo)),
    (0x54, (MetaType.SMPTEOffset, _smpte_offset)),
    (0x58, (MetaType.TimeSignature, _time_signature)),
    (0x59, (MetaType.KeySignature, _key_signature)),
    (0x7f, (MetaType.SequencerSpecific, _bytes)),
])

@dataclass
class MetaEvent:
    type: MetaType
    value: Any

def parse_sysex_event(buf):
    if buf[0] not in {0xf0, 0xf7}:
        return None
    buf = buf[1:]
    buf, length = parse_variable_length(buf)
    data = buf[:length]
    buf = buf[length:]
    return buf, SysexEvent(bytes(data))

def parse_meta_event(buf):
    if buf[0] != 0xff:
        return None
    type_n = buf[1]
    type, value_parser = meta_nums[type_n]
    buf, length = parse_variable_length(buf[2:])
    data = buf[:length]
    print("meta", length, bytes(data))
    buf = buf[length:]
    return buf, MetaEvent(type, value_parser(data))

midi_messages = dict([
    (0x80, (2, NoteOff)),
    (0x90, (2, NoteOn)),
    (0xa0, (2, PolyphonicKeyPressure)),
    (0xb0, (2, ControlChange)),
    (0xc0, (1, ProgramChange)),
    (0xd0, (1, ChannelPressure)),
    (0xe0, (2, PitchBendChange)),
    # ChannelMode handled specially
])

def parse_midi_event(buf):
    message_type = buf[0] & 0xf0
    #assert message_type != 0xf0, hex(message_type)
    if message_type not in midi_messages:
        return None
    channel = buf[0] & 0x0f
    message_length, cls = midi_messages[message_type]
    buf = buf[1:]
    data = buf[:message_length]
    # handle channel mode specially
    if cls is ControlChange and data[0] >= 121 and data[0] <= 127:
        # 0xb0 is overloaded for both control change and channel mode
        cls = ChannelMode
    message = cls(channel, *data)
    buf = buf[message_length:]
    return buf, message

def parse_event(buf):
    while True:
        b = buf[0]
        if (sysex := parse_sysex_event(buf)) is not None:
            buf, sysex = sysex
            return buf, sysex
        elif (meta := parse_meta_event(buf)) is not None:
            buf, meta = meta
            return buf, meta
        elif (midi := parse_midi_event(buf)) is not None:
            buf, midi = midi
            return buf, midi
        else:
            print(hex(buf[0]), file=sys.stderr)
            buf = buf[1:]
            while (buf[0] & 0x80) == 0:
                print(hex(buf[0]), file=sys.stderr)
                buf = buf[1:]

@dataclass
class Event:
    delta_time: int
    event: Union[MIDIEvent, SysexEvent, MetaEvent]

def parse_mtrk_event(buf):
    buf, delta_time = parse_variable_length(buf)
    buf, event = parse_event(buf)
    return buf, Event(delta_time, event)

def parse_track_chunk_type(buf):
    assert buf[0] == ord('M'), bytes(buf[0:4])
    assert buf[1] == ord('T'), bytes(buf[0:4])
    assert buf[2] == ord('r'), bytes(buf[0:4])
    assert buf[3] == ord('k'), bytes(buf[0:4])
    return buf[4:], None

@dataclass
class Track:
    events: list[Event]

def parse_track(buf):
    buf, _ = parse_track_chunk_type(buf)
    buf, length = parse_uint32(buf)
    offset = len(buf)
    events = []
    while (offset - len(buf)) < length:
        buf, event = parse_mtrk_event(buf)
        events.append(event)
    return buf, Track(events)

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

def parse_file(buf):
    buf, header = parse_header(buf)
    #print(header)
    assert header.ntrks > 0
    tracks = []
    for track_num in range(header.ntrks):
        buf, track = parse_track(buf)
        tracks.append(track)
        #print(f"track {track_num}:")
        for i, event in enumerate(track.events):
            #simulate_note(i, event)
            print(event)

    print("remaining data:", len(buf))

import sys
with open(sys.argv[1], 'rb') as f:
    b = memoryview(f.read())

parse_file(b)
