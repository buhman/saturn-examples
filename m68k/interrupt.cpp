#include <stdint.h>

#include "scsp.h"

extern void * _jojo_start __asm("_binary_jojo_11025_s16be_1ch_pcm_start");

static int32_t frame = 0x0;

constexpr int32_t tactl = 7;
constexpr int32_t frame_size = ((1 << tactl) * 256) / (44100 / 11025);
constexpr int32_t frame_count = 507150 / (frame_size * 2);
constexpr int32_t tima = 0;

static uint16_t slot_ix = 0;

extern "C"
void auto_vector_1(void) __attribute__ ((interrupt_handler));
void auto_vector_1(void)
{
  // reset TIMER_A interrupt
  scsp.reg.ctrl.SCIRE = INT__TIMER_A;
  scsp.reg.ctrl.TIMA = TIMA__TACTL(tactl) | TIMA__TIMA(tima);

  if (frame > frame_count) frame = 0;

  const uint16_t * jojo_start = reinterpret_cast<uint16_t *>(&_jojo_start);
  const uint32_t frame_addr = reinterpret_cast<uint32_t>(&jojo_start[frame * frame_size]);

  scsp_slot& slotp = scsp.reg.slot[(slot_ix - 1) & 31];
  slotp.LOOP = 0;
  slotp.LOOP |= LOOP__KYONEX;

  scsp.reg.ctrl.STATUS = STATUS__MSLC(slot_ix & 31);
  scsp_slot& slot = scsp.reg.slot[slot_ix & 31];
  // start address (bytes)
  slot.SA = SA__KYONB | SA__SA(frame_addr); // kx kb sbctl[1:0] ssctl[1:0] lpctl[1:0] 8b sa[19:0]
  slot.LSA = 0; // loop start address (samples)
  slot.LEA = frame_size; // loop end address (samples)
  slot.EG = EG__AR(0x1f) | EG__EGHOLD; // d2r d1r ho ar krs dl rr
  slot.FM = 0; // stwinh sdir tl mdl mdxsl mdysl
  slot.PITCH = PITCH__OCT(-2) | PITCH__FNS(0); // oct fns
  slot.LFO = 0; // lfof plfows
  slot.MIXER = MIXER__DISDL(0b101); // disdl dipan efsdl efpan

  slot.LOOP |= LOOP__KYONEX;

  scsp.ram.u32[0] = (0xdead << 16) | (slot_ix);
  scsp.ram.u32[1] = frame;
  frame++;
  slot_ix++;

  return;
}

void main()
{
  for (long i = 0; i < 807; i++) { asm volatile ("nop"); }   // wait for (way) more than 30µs

  for (int i = 0; i < 32; i++) {
    scsp_slot& slot = scsp.reg.slot[i];
    slot.SA = 0; // 32-bit access
    slot.LSA = 0;
    slot.LEA = 0;
    slot.EG = 0;
    slot.FM = 0; // 32-bit access
    slot.PITCH = 0;
    slot.LFO = 0;
    slot.MIXER = 0;
    slot.LOOP |= LOOP__KYONEX;
  }

  scsp.reg.ctrl.MIXER = MIXER__MEM4MB | MIXER__MVOL(0xf);

  // timer A is vector 1 (0b001)
  scsp.reg.ctrl.SCILV2 = 0;
  scsp.reg.ctrl.SCILV1 = 0;
  scsp.reg.ctrl.SCILV0 = SCILV__TIMER_A;

  // enable TIMER_A
  scsp.reg.ctrl.SCIRE = INT__TIMER_A;
  scsp.reg.ctrl.SCIEB = INT__TIMER_A;

  scsp.reg.ctrl.TIMA = TIMA__TACTL(tactl) | TIMA__TIMA(tima);

  asm volatile ("move.w #8192,%sr");
}
