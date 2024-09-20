from dataclasses import dataclass
import struct

# Table of index changes
index_table = [
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
]
assert len(index_table) == 16

# quantizer lookup table
step_size_table = [
     7      , 8     , 9     , 10    , 11    , 12    , 13    , 14    ,
     16     , 17    , 19    , 21    , 23    , 25    , 28    , 31    ,
     34     , 37    , 41    , 45    , 50    , 55    , 60    , 66    ,
     73     , 80    , 88    , 97    , 107   , 118   , 130   , 143   ,
     157    , 173   , 190   , 209   , 230   , 253   , 279   , 307   ,
     337    , 371   , 408   , 449   , 494   , 544   , 598   , 658   ,
     724    , 796   , 876   , 963   , 1060  , 1166  , 1282  , 1411  ,
     1552   , 1707  , 1878  , 2066  , 2272  , 2499  , 2749  , 3024  ,
     3327   , 3660  , 4026  , 4428  , 4871  , 5358  , 5894  , 6484  ,
     7132   , 7845  , 8630  , 9493  , 10442 , 11487 , 12635 , 13899 ,
     15289  , 16818 , 18500 , 20350 , 22385 , 24623 , 27086 , 29794 ,
     32767
]
assert len(step_size_table) == 89

@dataclass
class ADPCMState:
    predicted_sample: int
    index: int
    step_size: int

def encode_sample(state, sample):
    assert sample >= -32768 and sample <= 32767
    difference = sample - state.predicted_sample
    if difference >= 0:
        new_sample = 0b0000
    else:
        new_sample = 0b1000
        difference = -difference
    mask = 0b100

    temp_step_size = state.step_size
    # division through repeated subtraction
    for i in range(3):
        if difference >= temp_step_size:
            new_sample |= mask
            difference -= temp_step_size
        temp_step_size >>= 1
        mask >>= 1

    difference = 0
    if new_sample & 0b100:
        difference += state.step_size
    if new_sample & 0b010:
        difference += state.step_size >> 1
    if new_sample & 0b001:
        difference += state.step_size >> 2
    difference += state.step_size >> 3

    if new_sample & 0b1000:
        difference = -difference

    state.predicted_sample += difference
    if state.predicted_sample > 32767:
        state.predicted_sample = 32767
    elif state.predicted_sample < -32768:
        state.predicted_sample = -32768

    state.index += index_table[new_sample]
    if state.index < 0:
        state.index = 0
    elif state.index > 88:
        state.index = 88

    state.step_size = step_size_table[state.index]

    return new_sample

def sign_extend(value, bits):
    sign_bit = 1 << (bits - 1)
    return (value & (sign_bit - 1)) - (value & sign_bit)

def test_encode_sample():
    sample = sign_extend(0x873f, 16)
    state = ADPCMState(
        predicted_sample = sign_extend(0x8700, 16),
        index = 24,
        step_size = 73
    )

    new_sample = encode_sample(state, sample)
    assert new_sample == 3
    assert (state.predicted_sample & 0xffff) == 0x873f, hex(state.predicted_sample)
    assert state.index == 23
    assert state.step_size == step_size_table[23]

test_encode_sample()

def decode_sample(state, sample):
    difference = 0
    if sample & 0b100:
        difference += state.step_size
    if sample & 0b010:
        difference += state.step_size >> 1
    if sample & 0b001:
        difference += state.step_size >> 2
    difference += state.step_size >> 3

    if sample & 0b1000:
        difference = -difference

    state.predicted_sample += difference
    if state.predicted_sample > 32767:
        state.predicted_sample = 32767
    elif state.predicted_sample < -32768:
        state.predicted_sample = -32768

    state.index += index_table[sample]
    if state.index < 0:
        state.index = 0
    elif state.index > 88:
        state.index = 88
    state.step_size = step_size_table[state.index]

    return state.predicted_sample

def test_decode_sample():
    sample = 3
    state = ADPCMState(
        predicted_sample = sign_extend(0x8700, 16),
        index = 24,
        step_size = 73
    )

    new_sample = decode_sample(state, sample)
    assert new_sample == sign_extend(0x873f, 16)
    assert state.index == 23
    assert state.step_size == step_size_table[23]

test_decode_sample()

with open('ecclesia.pcm', 'rb') as f:
    pcm = f.read()

def nibbles(buf):
    state = ADPCMState(
        0, 0, 7
    )
    byte = 0
    for i in range(len(buf) // 2):
        sample, = struct.unpack('>h', buf[i * 2:i * 2+2])
        new_sample = encode_sample(state, sample)
        assert new_sample >= 0 and new_sample <= 15
        if i % 2 == 0:
            byte = new_sample
        else:
            byte |= (new_sample << 4)
            yield byte

with open('ecclesia.adpcm', 'wb') as f:
    f.write(bytes(nibbles(pcm)))

with open('ecclesia.adpcm', 'rb') as f:
    adpcm = f.read()

def unnibbles(buf):
    state = ADPCMState(
        0, 0, 7
    )
    for i in range(len(buf) * 2):
        nib = (buf[i // 2] >> (4 * (i % 2))) & 0xf
        new_sample = decode_sample(state, nib)
        yield struct.pack('>h', new_sample)

with open('ecclesia.adpcm.pcm', 'wb') as f:
    for b in unnibbles(adpcm):
        f.write(b)
