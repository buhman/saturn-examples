#include "cdc/serial.h"
#include "scu.h"
#include "vdp2.h"

#include "scu-dsp/div10.dsp.h"

void main()
{
  scu.reg.PPAF = 0; // stop execution
  scu.reg.PPAF = (1 << 15) | 0; // reset PC

  uint32_t * program = (uint32_t *)&_binary_scu_dsp_div10_dsp_start;
  int program_length = ((int)&_binary_scu_dsp_div10_dsp_size) / 4;

  for (int i = 0; i < program_length; i++) {
    uint32_t p = program[i];
    scu.reg.PPD = p;
    serial_integer(p, 8, '\n');
  }

  scu.reg.PPAF = (1 << 16); // execute

  int end_flag = 0;
  while (end_flag == 0) {
    end_flag = (scu.reg.PPAF >> 18) & 1;
  }

  scu.reg.PPAF = 0;
  scu.reg.PDA = 64 * 3; // m3[0]

  serial_string("answer:\n");
  for (int i = 0; i < 8; i++) {
    serial_integer(scu.reg.PDD, 8, '\n');
  }
}
