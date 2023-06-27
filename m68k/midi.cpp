#include <cstdint>
#include <tuple>

#include "scsp.h"
#include "../math/fp.hpp"
#include "../midi/parse.hpp"
#include "../common/copy.hpp"

extern void * _sine_start __asm("_binary_sine_44100_s16be_1ch_100sample_pcm_start");
//extern void * _midi_start __asm("_binary_midi_test_c_major_scale_mid_start");
extern void * _midi_start __asm("_binary_f2_mid_start");

uint16_t
midi_note_to_oct_fns(const int8_t midi_note)
{
  static const uint16_t _cent_to_fns[] = {
    0x0,
    0x3d,
    0x7d,
    0xc2,
    0x10a,
    0x157,
    0x1a8,
    0x1fe,
    0x25a,
    0x2ba,
    0x321,
    0x38d,
  };

  const int8_t a440_note = midi_note - 69;
  const int8_t a440_note_d = (a440_note < 0) ? a440_note - 11 : a440_note;
  const int8_t div12 = a440_note_d / static_cast<int8_t>(12);
  const int8_t mod12 = a440_note % static_cast<int8_t>(12);

  const uint16_t oct = div12;
  const uint16_t cent = (a440_note < 0) ? 12 + mod12 : mod12;
  const uint16_t fns = _cent_to_fns[cent];

  return PITCH__OCT(oct) | PITCH__FNS(fns);
}

// maximum delay of 3258 days
using fp48_16 = fp<uint64_t, uint64_t, 16>;

constexpr uint8_t tactl = 0; // F/128
constexpr uint8_t tima = 0x00;

constexpr fp48_16 increment_ms{5804, 64657}; //

struct midi_state {
  uint8_t const * buf;
  midi::header_t header;
  struct track {
    uint8_t const * start;
    int32_t length;
  } current_track;

  struct midi {
    uint32_t tempo;
  } midi;

  fp48_16 delta_time_ms;
};

static midi_state state = {0};

[[noreturn]]
void error()
{
  const uint32_t sine_start = reinterpret_cast<uint32_t>(&_sine_start);

  {
    scsp_slot& slot = scsp.reg.slot[0];
    // start address (bytes)
    slot.SA = SA__KYONB | SA__LPCTL__NORMAL | SA__SA(sine_start); // kx kb sbctl[1:0] ssctl[1:0] lpctl[1:0] 8b sa[19:0]
    slot.LSA = 0; // loop start address (samples)
    slot.LEA = 99; // loop end address (samples)
    slot.EG = EG__EGHOLD; // d2r d1r ho ar krs dl rr
    slot.FM = 0; // stwinh sdir tl mdl mdxsl mdysl
    //slot.PITCH = PITCH__OCT(0) | PITCH__FNS(0); // oct fns
    slot.PITCH = 0x78c2;
    slot.LFO = 0; // lfof plfows
    slot.MIXER = MIXER__DISDL(0b110); // disdl dipan efsdl efpan
  }

  {
    scsp_slot& slot = scsp.reg.slot[1];
    // start address (bytes)
    slot.SA = SA__KYONB | SA__LPCTL__NORMAL | SA__SA(sine_start); // kx kb sbctl[1:0] ssctl[1:0] lpctl[1:0] 8b sa[19:0]
    slot.LSA = 0; // loop start address (samples)
    slot.LEA = 99; // loop end address (samples)
    slot.EG = EG__EGHOLD; // d2r d1r ho ar krs dl rr
    slot.FM = 0; // stwinh sdir tl mdl mdxsl mdysl
    slot.PITCH = PITCH__OCT(1) | PITCH__FNS(0); // oct fns
    slot.LFO = 0; // lfof plfows
    slot.MIXER = MIXER__DISDL(0b110); // disdl dipan efsdl efpan
  }

  scsp.reg.slot[0].LOOP |= LOOP__KYONEX;

  while (1);
}

fp48_16 delay_ms(uint32_t delta_time)
{
  return {(delta_time * state.midi.tempo) /
	  state.header.division.metrical.ticks_per_quarter_note};
}

static int _event = 0;

static volatile int timer_fired = 0;

extern "C"
void auto_vector_1(void) __attribute__ ((interrupt_handler));
void auto_vector_1(void)
{
   // reset TIMER_A interrupt
  scsp.reg.ctrl.SCIRE = INT__TIMER_A;
  scsp.reg.ctrl.TIMA = TIMA__TACTL(tactl) | TIMA__TIMA(tima);

  timer_fired = 1;
}

struct vs {
  int8_t slot_ix;
  int8_t count;
};

static int32_t slot_alloc;
static struct vs voice_slot[16][128];

#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
int8_t alloc_slot()
{
  for (int i = 0; i < 32; i++) {
    uint32_t bit = (1 << i);
    if ((slot_alloc & bit) == 0) {
      slot_alloc |= bit;
      return i;
    }
  }
  return -1;
}
#pragma GCC pop_options

void free_slot(int8_t i)
{
  slot_alloc &= ~(1 << i);
}

void midi_step()
{
  const uint32_t sine_start = reinterpret_cast<uint32_t>(&_sine_start);
  // 11.2896 MHz

  // 2902.4943 µs per interrupt this gives us 32768 cycles per
  // interrupt, which *might* be roomy enough.

  int kyonex = 0;

  while (true) {
    scsp.ram.u32[0] = _event;
    scsp.ram.u32[1] = state.delta_time_ms.value >> 16;

    if (!(state.buf - state.current_track.start < state.current_track.length))
      return;

    auto mtrk_event_o = midi::parse::mtrk_event(state.buf);
    if (!mtrk_event_o) error();
    midi::mtrk_event_t mtrk_event;
    uint8_t const * buf;
    std::tie(buf, mtrk_event) = *mtrk_event_o;

    scsp.ram.u32[2] = delay_ms(mtrk_event.delta_time).value >> 16;

    fp48_16 delta_time_ms{0};
    if (mtrk_event.delta_time != 0 &&
	state.delta_time_ms.value < (delta_time_ms = delay_ms(mtrk_event.delta_time)).value)
      break;
    else
      state.buf = buf;

    _event++;

    switch (mtrk_event.event.type) {
    case midi::event_t::type_t::midi:
      {
	midi::midi_event_t& midi_event = mtrk_event.event.event.midi;

	switch (midi_event.type) {
	case midi::midi_event_t::type_t::note_on:
	  {
	    struct vs& v = voice_slot[midi_event.data.note_on.channel][midi_event.data.note_on.note];
	    if (v.slot_ix == -1) {
	      v.slot_ix = alloc_slot();
	    }
	    v.count++;

	    scsp_slot& slot = scsp.reg.slot[v.slot_ix];

	    scsp.ram.u32[3] = midi_event.data.note_on.note;

	    //slot.SA = SA__KYONB | SA__LPCTL__NORMAL | SA__SA(sine_start);
	    slot.LOOP = LOOP__KYONB | LOOP__LPCTL__NORMAL | SAH__SA(sine_start);
	    slot.SAL = SAL__SA(sine_start);
	    slot.LSA = 0;
	    slot.LEA = 99;
	    slot.EG = EG__EGHOLD; //EG__AR(0x0f) | EG__D1R(0x4) | EG__D2R(0x4) | EG__RR(0x1f);
	    slot.FM = 0;
	    slot.PITCH = midi_note_to_oct_fns(midi_event.data.note_on.note);
	    slot.LFO = 0;
	    slot.MIXER = MIXER__DISDL(0b110);

	    if (v.count == 1)
	      kyonex = 1;
	  }
	  break;
	case midi::midi_event_t::type_t::note_off:
	  {
	    struct vs& v = voice_slot[midi_event.data.note_on.channel][midi_event.data.note_on.note];
	    if (v.slot_ix < 0) error();

	    v.count -= 1;
	    if (v.count == 0) {
	      free_slot(v.slot_ix);
	      scsp_slot& slot = scsp.reg.slot[v.slot_ix];
	      v.slot_ix = -1;
	      slot.LOOP = 0;
	      scsp.reg.slot[0].SA |= SA__KYONEX;
	      kyonex = 1;
	    }
	  }
	  break;
	default:
	  break;
	}
      }
      break;

    default:
      break;
    }

    state.delta_time_ms -= delta_time_ms;
  }

  if (kyonex) {
    scsp.reg.slot[0].SA |= SA__KYONEX;
  }

  state.delta_time_ms += increment_ms;
}


void init_midi()
{
  state.buf = reinterpret_cast<uint8_t const *>(&_midi_start);

  auto header_o = midi::parse::header(state.buf);
  if (!header_o) error();
  std::tie(state.buf, state.header) = *header_o;

  auto track_o = midi::parse::track(state.buf);
  if (!track_o) error();
  std::tie(state.buf, state.current_track.length) = *track_o;
  state.current_track.start = state.buf;

  state.delta_time_ms = fp48_16{0};
  state.midi.tempo = 500000; // default tempo

  slot_alloc = 0;
  for (int j = 0; j < 16; j++) {
    for (int i = 0; i < 128; i++) {
      voice_slot[j][i].slot_ix = -1;
      voice_slot[j][i].count = 0;
    }
  }
}

void main()
{
  _event = 0;
  scsp.ram.u32[0] = _event;

  for (long i = 0; i < 3200; i++) { asm volatile ("nop"); }   // wait for (way) more than 30µs

  init_midi();

  // timer A is vector 1 (0b001)
  scsp.reg.ctrl.SCILV2 = 0;
  scsp.reg.ctrl.SCILV1 = 0;
  scsp.reg.ctrl.SCILV0 = SCILV__TIMER_A;

  // enable TIMER_A
  scsp.reg.ctrl.SCIRE = INT__TIMER_A;
  scsp.reg.ctrl.SCIEB = INT__TIMER_A;

  scsp.reg.ctrl.TIMA = TIMA__TACTL(tactl) | TIMA__TIMA(tima);

  asm volatile ("move.w #8192,%sr");

  while (1) {
    while (timer_fired == 0) {
      asm volatile ("nop");
    }

    timer_fired = 0;
    midi_step();
  }
}
