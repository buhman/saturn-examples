#include <stdint.h>

#include "scsp.h"

extern void * _sine_start __asm("_binary_sine_44100_s16be_1ch_1sec_pcm_start");

void main()
{
  for (long i = 0; i < 807; i++) { asm volatile ("nop"); }   // wait for (way) more than 30µs

  scsp.reg.ctrl.MIXER = MIXER__MEM4MB | MIXER__MVOL(0xf);

  const uint32_t sine_start = reinterpret_cast<uint32_t>(&_sine_start);

  scsp_slot& slot = scsp.reg.slot[0];
  slot.LOOP = 0;
  slot.LOOP |= LOOP__KYONEX;

  // start address (bytes)
  slot.SA = SA__KYONB | SA__LPCTL__NORMAL | SA__SA(sine_start); // kx kb sbctl[1:0] ssctl[1:0] lpctl[1:0] 8b sa[19:0]
  slot.LSA = 0; // loop start address (samples)
  slot.LEA = 44100; // loop end address (samples)
  slot.EG = EG__AR(0x1f) | EG__EGHOLD; // d2r d1r ho ar krs dl rr
  slot.FM = 0; // stwinh sdir tl mdl mdxsl mdysl
  slot.PITCH = PITCH__OCT(0) | PITCH__FNS(0); // oct fns
  slot.LFO = 0; // lfof plfows
  slot.MIXER = MIXER__DISDL(0b101); // disdl dipan efsdl efpan

  slot.LOOP |= LOOP__KYONEX;
}
