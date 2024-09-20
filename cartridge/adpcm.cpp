#include <stdint.h>

#include "vdp1.h"
#include "vdp2.h"
#include "scsp.h"
#include "smpc.h"

#include "../common/copy.hpp"

extern void * _ecclesia_adpcm_start __asm("_binary_ecclesia_adpcm_start");
extern void * _ecclesia_adpcm_size __asm("_binary_ecclesia_adpcm_size");

// table of index changes
static const int8_t index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

// quantizer lookup table
static const int16_t step_size_table[89] = {
     7      , 8     , 9     , 10    , 11    , 12    , 13    , 14    ,
     16     , 17    , 19    , 21    , 23    , 25    , 28    , 31    ,
     34     , 37    , 41    , 45    , 50    , 55    , 60    , 66    ,
     73     , 80    , 88    , 97    , 107   , 118   , 130   , 143   ,
     157    , 173   , 190   , 209   , 230   , 253   , 279   , 307   ,
     337    , 371   , 408   , 449   , 494   , 544   , 598   , 658   ,
     724    , 796   , 876   , 963   , 1060  , 1166  , 1282  , 1411  ,
     1552   , 1707  , 1878  , 2066  , 2272  , 2499  , 2749  , 3024  ,
     3327   , 3660  , 4026  , 4428  , 4871  , 5358  , 5894  , 6484  ,
     7132   , 7845  , 8630  , 9493  , 10442 , 11487 , 12635 , 13899 ,
     15289  , 16818 , 18500 , 20350 , 22385 , 24623 , 27086 , 29794 ,
     32767
};


struct adpcm_state {
  int32_t predicted_sample;
  int32_t index;
  int32_t step_size;
};

static struct adpcm_state state;

void __attribute__ ((noinline)) init_state()
{
  state.predicted_sample = 0;
  state.index = 0;
  state.step_size = 7;
}

int16_t decode_sample(int sample)
{
  int difference = 0;
  if (sample & 0b100)
    difference += state.step_size;
  if (sample & 0b010)
    difference += state.step_size >> 1;
  if (sample & 0b001)
    difference += state.step_size >> 2;
  difference += state.step_size >> 3;

  if (sample & 0b1000) {
    difference = -difference;
  }

  state.predicted_sample += difference;
  if (state.predicted_sample > 32767)
    state.predicted_sample = 32767;
  if (state.predicted_sample < -32768)
    state.predicted_sample = -32768;

  state.index += index_table[sample];
  if (state.index < 0)
    state.index = 0;
  if (state.index > 88)
    state.index = 88;
  state.step_size = step_size_table[state.index];

  return state.predicted_sample;
}

void snd_init()
{
  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDOFF;
  while (smpc.reg.OREG[31].val != OREG31__SNDOFF);

  scsp.reg.ctrl.MIXER = MIXER__MEM4MB;

  volatile uint32_t * dsp_steps = reinterpret_cast<volatile uint32_t *>(&(scsp.reg.dsp.STEP[0].MPRO[0]));
  fill<volatile uint32_t>(dsp_steps, 0, (sizeof (scsp.reg.dsp.STEP)));

  for (int i = 0; i < 32; i++) {
    scsp.reg.slot[i].SA = 0;
    scsp.reg.slot[i].MIXER = 0;
  }

  scsp.reg.ctrl.MIXER = MIXER__MEM4MB | MIXER__MVOL(0xf);
}

void __attribute__ ((noinline)) decode_chunk(volatile uint16_t * dst, const uint8_t * src, int samples)
{
  for (int i = 0; i < samples / 2; i++) {
    uint8_t b = src[i];
    int nib0 = (b >> 0) & 0xf;
    int nib1 = (b >> 4) & 0xf;
    dst[i * 2 + 0] = decode_sample(nib0);
    dst[i * 2 + 1] = decode_sample(nib1);
  }
}

void snd_step()
{
  const uint8_t * buf = reinterpret_cast<uint8_t*>(&_ecclesia_adpcm_start);
  const uint32_t size = reinterpret_cast<uint32_t>(&_ecclesia_adpcm_size);
  constexpr uint32_t chunk_samples = 16384 / 2;
  decode_chunk(&scsp.ram.u16[0], buf, chunk_samples);

  scsp_slot& slot = scsp.reg.slot[0];
  // start address (bytes)
  slot.SA = SA__KYONB | SA__LPCTL__NORMAL | SA__SA(0); // kx kb sbctl[1:0] ssctl[1:0] lpctl[1:0] 8b sa[19:0]
  slot.LSA = 0; // loop start address (samples)
  slot.LEA = chunk_samples * 2; // loop end address (samples)
  //slot.EG = EG__EGHOLD; // d2r d1r ho ar krs dl rr
  slot.EG = EG__AR(0x1F) | EG__D1R(0x00) | EG__D2R(0x00) | EG__RR(0x1F);
  slot.FM = 0; // stwinh sdir tl mdl mdxsl mdysl
  slot.PITCH = PITCH__OCT(-1) | PITCH__FNS(0); // oct fns
  slot.LFO = 0; // lfof plfows
  slot.MIXER = MIXER__DISDL(0b110); // disdl dipan efsdl efpan

  slot.LOOP |= LOOP__KYONEX;

  uint32_t offset = 1;
  uint32_t chunk = 1;

  constexpr uint32_t timer_a_interrupt = (1 << 6);
  scsp.reg.ctrl.TIMA = TIMA__TACTL(7)
                     | TIMA__TIMA(128);
  scsp.reg.ctrl.SCIRE = timer_a_interrupt;

  while (1) {
    decode_chunk(&scsp.ram.u16[chunk_samples * chunk],
		 &buf[(chunk_samples * offset) / 2],
		 chunk_samples);
    chunk = !chunk;
    offset = offset + 1;
    if (chunk_samples * offset > size * 2) {
      offset = 0;
    }

    /*
    uint32_t sample = 0;
    const uint32_t target = chunk_samples * 2;
    constexpr uint32_t sample_interval = (1 << 10);
    while (sample < target) {
      scsp.reg.ctrl.SCIRE = sample_interval;
      while (!(scsp.reg.ctrl.SCIPD & sample_interval));
      sample++;
    }
    */
    while (!(scsp.reg.ctrl.SCIPD & timer_a_interrupt));
    scsp.reg.ctrl.TIMA = TIMA__TACTL(7)
                       | TIMA__TIMA(128);
    scsp.reg.ctrl.SCIRE = timer_a_interrupt;
  }
}

extern "C"
void main() __attribute__((section(".text.main")));
void main()
{
  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  // VDP2 User's Manual:
  // "When sprite data is in an RGB format, sprite register 0 is selected"
  // "When the value of a priority number is 0h, it is read as transparent"
  //
  // The power-on value of PRISA is zero. Set the priority for sprite register 0
  // to some number greater than zero, so that the color data is not interpreted
  // as "transparent".
  vdp2.reg.PRISA = PRISA__S0PRIN(1); // Sprite register 0 Priority Number

  /* TVM settings must be performed from the second H-blank IN interrupt after the
  V-blank IN interrupt to the H-blank IN interrupt immediately after the V-blank
  OUT interrupt. */
  // "normal" display resolution, 16 bits per pixel, 512x256 framebuffer
  vdp1.reg.TVMR = TVMR__TVM__NORMAL;

  // swap framebuffers every 1 cycle; non-interlace
  vdp1.reg.FBCR = 0;

  // erase upper-left coordinate
  vdp1.reg.EWLR = EWLR__16BPP_X1(0) | EWLR__Y1(0);

  // erase lower-right coordinate
  vdp1.reg.EWRR = EWRR__16BPP_X3(319) | EWRR__Y3(239);

  // during a framebuffer erase cycle, write the color "black" to each pixel
  const uint16_t black = 1 << 15 | 0x00ff;
  vdp1.reg.EWDR = black;

  vdp1.vram.cmd[0].CTRL = CTRL__END;

  init_state();
  snd_init();
  snd_step();

  const uint16_t blue = 1 << 15 | 0x7f00;
  vdp1.reg.EWDR = blue;

  vdp2.reg.BGON = 0;

  // start drawing (execute the command list) on every frame
  vdp1.reg.PTMR = PTMR__PTM__FRAME_CHANGE;

  while (1);
}
