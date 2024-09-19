#include <cstdint>
#include <tuple>

#include "scsp.h"
#include "../math/fp.hpp"
#include "../midi/parse.hpp"
#include "../common/copy.hpp"

extern void * _sine_start __asm("_binary_sine_44100_s16be_1ch_100sample_pcm_start");
extern void * _midi_start __asm("_binary_midi_test_c_major_scale_mid_start");
//extern void * _midi_start __asm("_binary_f2_mid_start");

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

	    slot.SA = SA__KYONB | SA__LPCTL__NORMAL | SA__SA(sine_start);
	    slot.LSA = 0;
	    slot.LEA = 100;
	    slot.EG = EG__AR(0x1f) | EG__D1R(0x0) | EG__D2R(0x0) | EG__RR(0x1f);
	    slot.FM = 0;
	    slot.PITCH = midi_note_to_oct_fns(midi_event.data.note_on.note);
	    slot.LFO = 0;
	    slot.MIXER = MIXER__DISDL(0b101);

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
	      //kyonex = 1;
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
