#include "cdc/serial.h"
#include "scu.h"

#include "scu-dsp/advent/2020/day1/runner.dsp.h"
#include "scu-dsp/advent/2020/day1/input.bin.h"

#include "scu-dsp/vdp2_text.hpp"

void main()
{
  vdp2_text_init();

  scu.reg.PPAF = 0; // stop execution
  scu.reg.PPAF = (1 << 15) | 0; // reset PC

  uint32_t * program = (uint32_t *)&_binary_scu_dsp_advent_2020_day1_runner_dsp_start;
  int program_length = ((int)&_binary_scu_dsp_advent_2020_day1_runner_dsp_size) / 4;

  for (int i = 0; i < program_length; i++) {
    uint32_t p = program[i];
    scu.reg.PPD = p;
    //serial_integer(p, 8, '\n');
  }

  uint32_t * data = (uint32_t *)&_binary_scu_dsp_advent_2020_day1_input_bin_start;
  int data_length = ((int)&_binary_scu_dsp_advent_2020_day1_input_bin_size) / 4;

  scu.reg.PDA = 64 * 0; // m0[0]
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < data_length; i++) {
      uint32_t d = data[i];
      scu.reg.PDD = d;
    }
  }

  scu.reg.PPAF = (1 << 15) | 0; // reset PC
  scu.reg.PPAF = (1 << 16); // execute

  int end_flag = 0;
  while (end_flag == 0) {
    end_flag = (scu.reg.PPAF >> 18) & 1;
  }

  scu.reg.PPAF = 0;
  scu.reg.PDA = 64 * 3 + 56; // m3[56]

  serial_string("answer:\n");
  for (int i = 0; i < 8; i++) {
    serial_integer(scu.reg.PDD, 8, '\n');
  }
}
