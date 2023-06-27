from dataclasses import dataclass

def note_freq(n):
    return 440 * 2 ** ((n-69)/12)

@dataclass
class State:
    tempo: int = 500000 # µS/qn
    division: int = 96 # ticks/qn

state = State()

def delay(dt # microseconds
          ) -> int: # in microseconds
    return int((dt * state.tempo) / state.division)

print(delay(96))
