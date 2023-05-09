#include <stdint.h>

#include "vdp2.h"

#include "palette.hpp"

constexpr inline uint16_t rgb15_gray(uint32_t intensity)
{
  return ((intensity & 31) << 10)  // blue
       | ((intensity & 31) << 5 )  // green
       | ((intensity & 31) << 0 ); // red
}

namespace palette {

void vdp2_cram_32grays(uint32_t colors_per_palette, uint32_t color_bank_index)
{
  /* generate a palette of 32 grays */
  uint16_t * table = &vdp2.cram.u16[colors_per_palette * color_bank_index];

  for (uint32_t i = 0; i <= 31; i++) {
    table[i] = rgb15_gray(i);
  }
}

}
