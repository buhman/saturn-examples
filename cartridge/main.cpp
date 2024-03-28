#include "vdp1.h"
#include "vdp2.h"
#include "scsp.h"
#include "smpc.h"

#include "../common/copy.hpp"

extern void * _wc3_pcm_start __asm("_binary_wc3_pcm_start");
extern void * _wc3_pcm_size __asm("_binary_wc3_pcm_size");

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

void snd_step()
{
  const uint16_t * buf = reinterpret_cast<uint16_t*>(&_wc3_pcm_start);
  const uint32_t size = reinterpret_cast<uint32_t>(&_wc3_pcm_size);
  constexpr uint32_t chunk_samples = 16384 / 2;
  constexpr uint32_t chunk_size = chunk_samples * 2;
  copy<uint16_t>(&scsp.ram.u16[0], buf, chunk_size);

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
    copy<uint16_t>(&scsp.ram.u16[(chunk_size * chunk) / 2], &buf[(chunk_size * offset) / 2], chunk_size);
    chunk = !chunk;
    offset = offset + 1;
    if ((offset * (chunk_size + 1)) > size) {
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

  snd_init();
  snd_step();

  vdp2.reg.BGON = 0;

  // start drawing (execute the command list) on every frame
  vdp1.reg.PTMR = PTMR__PTM__FRAME_CHANGE;

  while (1);
}
