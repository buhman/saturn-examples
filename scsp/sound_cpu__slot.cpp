#include <stdint.h>

#include "smpc.h"
#include "scsp.h"

#include "../common/copy.hpp"

extern void * _m68k_start __asm("_binary_m68k_slot_bin_start");
extern void * _m68k_size __asm("_binary_m68k_slot_bin_size");

void main()
{
  /* SEGA SATURN TECHNICAL BULLETIN # 51

     The document suggests that Sound RAM is (somewhat) preserved
     during SNDOFF.
   */
  
  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDOFF;
  while (smpc.reg.oreg[31] != OREG31__SNDOFF);

  scsp.reg.ctrl.MIXER = MIXER__MEM4MB;
  
  uint32_t * m68k_main_start = reinterpret_cast<uint32_t*>(&_m68k_start);
  uint32_t m68k_main_size = reinterpret_cast<uint32_t>(&_m68k_size);
  copy<uint32_t>(&scsp.ram.u32[0], m68k_main_start, m68k_main_size);
  
  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDON;
  while (smpc.reg.oreg[31] != OREG31__SNDON);
  
  // do nothing while the sound CPU manipulates the SCSP
}

extern "C"
void start(void)
{
  main();
  while (1);
}
