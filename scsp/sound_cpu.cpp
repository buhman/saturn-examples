#include <stdint.h>

#include "smpc.h"
#include "scsp.h"
#include "vdp2.h"

#include "../common/copy.hpp"
#include "../common/vdp2_func.hpp"

extern void * _binary_m68k_slot_bin_start __asm("_binary_m68k_slot_bin_start");
extern void * _binary_m68k_slot_bin_size __asm("_binary_m68k_slot_bin_size");

void main()
{
  v_blank_in();

  /*
  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDOFF;
  while (smpc.reg.oreg[31] != 0b00000111);
  
  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDON;
  while (smpc.reg.oreg[31] != 0b00000110);
  */

  scsp.reg.ctrl.MIXER = MIXER__MEM4MB | MIXER__DAC18B | MIXER__MVOL(0xf);
  
  uint32_t * m68k_main_start = (uint32_t *)&_binary_m68k_slot_bin_start;
  uint32_t m68k_main_size = (uint32_t)&_binary_m68k_slot_bin_size;
  copy<uint32_t>(&scsp.ram.u32[0], m68k_main_start, m68k_main_size);
    
  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDOFF;
  while (smpc.reg.oreg[31] != 0b00000111);
  
  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDON;
  while (smpc.reg.oreg[31] != 0b00000110);
  
  // do nothing while the sound CPU manipulates the SCSP
}

extern "C"
void start(void)
{
  main();
  while (1);
}
