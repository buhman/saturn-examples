#include <cstdint>

#include "scsp.h"

static uint32_t ram_ix = 0;

static uint16_t * debug_buf = &scsp.ram.u16[0x080000 / 2];
static uint16_t * debug_length = &scsp.ram.u16[(0x080000 / 4) - 2];

void main()
{
  *debug_length = 0;

  while (true) {
    uint16_t midiu = scsp.reg.ctrl.MIDIU;
    uint16_t mibuf = MIDIU__MIBUF(midiu);
    uint16_t miemp = midiu & MIDIU__MIEMP;
    if (miemp) continue;

    (*debug_length)++;
    (*debug_buf) = mibuf;
    debug_buf++;
  }
}
