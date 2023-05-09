#include <stdint.h>
#include "vdp1.h"
#include "vdp2.h"
#include "smpc.h"
#include "sh2.h"
#include "scu.h"

#include "../common/keyboard.hpp"
#include "../common/font.hpp"
#include "../common/draw_font.hpp"
#include "../common/palette.hpp"
#include "../common/vdp2_func.hpp"

#include "wordle.hpp"
#include "draw.hpp"

extern void * _dejavusans_start __asm("_binary_res_dejavusansmono_font_bin_start");

struct draw_state {
  uint32_t cmd_ix;
  struct draw_font::state font;
};

static struct draw_state draw_state;

static struct wordle::screen wordle_state;

#define assert(n) if ((n) == 0) while (1);

enum intback_fsm {
  PORT_STATUS = 0,
  PERIPHERAL_ID,
  DATA1,
  DATA2,
  DATA3,
  DATA4,
  FSM_NEXT
};

struct intback_state {
  int fsm;
  int controller_ix;
  int port_ix;
};

static intback_state intback;
static int oreg_ix;

void smpc_int(void) __attribute__ ((interrupt_handler));
void smpc_int(void) {
  scu.reg.IST &= ~(IST__SMPC);
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);

  if ((smpc.reg.SR & SR__PDL) != 0) {
    // PDL == 1; 1st peripheral data
    oreg_ix = 0;
    intback.controller_ix = 0;
    intback.port_ix = 0;
    intback.fsm = PORT_STATUS;
  }

  int port_connected = 0;
  int data_size = 0;
  int peripheral_type = 0;
  (void)peripheral_type;
  int kbd_bits = 0;

  /*
    This intback handling is oversimplified:

    - up to 2 controllers may be connected
    - multitaps are not parsed correctly
   */
  while (oreg_ix < 31) {
    reg8 const& oreg = smpc.reg.oreg[oreg_ix++];
    switch (intback.fsm++) {
    case PORT_STATUS:
      port_connected = (PORT_STATUS__CONNECTORS(oreg) == 1);
      if (port_connected) {
        assert(PORT_STATUS__MULTITAP_ID(oreg) == 0xf);
      } else {
        intback.fsm = FSM_NEXT;
      }
      break;
    case PERIPHERAL_ID:
      peripheral_type = PERIPHERAL_ID__TYPE(oreg);
      data_size = PERIPHERAL_ID__DATA_SIZE(oreg);
      break;
    case DATA1:
      {
        //controller_state& c = intback.controller[intback.controller_ix];
      }
      break;
    case DATA2:
      break;
    case DATA3:
      kbd_bits = oreg & 0b1111;
      break;
    case DATA4:
      {
        uint32_t keysym = oreg;

         if (kbd_bits & 0b1000) { // Make
          enum keysym k = scancode_to_keysym(keysym);
          char16_t c = keysym_to_char16(k);
          (void)c;

        } else if (kbd_bits & 0b0001) { // Break

        }
      }
      break;
    default:
      assert(0);
      break;
    }

    if ((intback.fsm >= PERIPHERAL_ID && (data_size--) < 0) || intback.fsm == FSM_NEXT) {
      if (intback.port_ix == 1)
        break;
      else {
        intback.port_ix++;
        intback.controller_ix++;
        intback.fsm = PORT_STATUS;
      }
    }
  }

  if ((smpc.reg.SR & SR__NPE) != 0) {
    smpc.reg.ireg[0] = INTBACK__IREG0__CONTINUE;
  } else {
    smpc.reg.ireg[0] = INTBACK__IREG0__BREAK;
  }
}

// rendering

void draw_char(uint8_t c, int32_t x, int32_t y)
{
  draw_font::single_character(draw_state.font,
                              draw_state.cmd_ix++,
                              c,
                              (x) << 6,
                              (y + 30) << 6);

  vdp1.vram.cmd[draw_state.cmd_ix].CTRL = CTRL__END;
}

void render()
{
  draw_state.cmd_ix = 2;


  wordle_state.rows[0].letters[0] = 't';

  wordle::draw::guesses(wordle_state, &draw_char);
}

void v_blank_in_int(void) __attribute__ ((interrupt_handler));
void v_blank_in_int() {
  scu.reg.IST &= ~(IST__V_BLANK_IN);
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);

  sh2.reg.FRC.H = 0;
  sh2.reg.FRC.L = 0;
  sh2.reg.FTCSR = 0; // clear flags

  // render

  render();

  // wait at least 300us, as specified in the SMPC manual.
  // It appears reading FRC.H is mandatory and *must* occur before FRC.L on real
  // hardware.
  while ((sh2.reg.FTCSR & FTCSR__OVF) == 0 && sh2.reg.FRC.H == 0 && sh2.reg.FRC.L < 63);

  if ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) != 0) {
    // on real hardware, SF contains uninitialized garbage bits other than the
    // lsb.
    while ((smpc.reg.SF & 1) != 0);

    smpc.reg.SF = 0;

    smpc.reg.ireg[0] = INTBACK__IREG0__STATUS_DISABLE;
    smpc.reg.ireg[1] = ( INTBACK__IREG1__PERIPHERAL_DATA_ENABLE
                       | INTBACK__IREG1__PORT2_15BYTE
                       | INTBACK__IREG1__PORT1_15BYTE
                       );
    smpc.reg.ireg[2] = INTBACK__IREG2__MAGIC;

    smpc.reg.COMREG = COMREG__INTBACK;
  }
}

uint32_t init_font(uint32_t top)
{
  // 256 is the number of colors in the color palette, not the number of grays
  // that are used by the font.
  constexpr uint32_t colors_per_palette = 256;
  constexpr uint32_t color_bank_index = 0; // completely random and arbitrary value

  palette::vdp2_cram_32grays(colors_per_palette, color_bank_index);
  // For color bank color, COLR is concatenated bitwise with pixel data. See
  // Figure 6.17 in the VDP1 manual.
  draw_state.font.color_address = color_bank_index << 8;

  top = font_data(&_dejavusans_start, top, draw_state.font);

  return top;
}

void main()
{
  uint32_t top = (sizeof (union vdp1_vram));
  top = init_font(top);

  // wait for the beginning of a V blank
  v_blank_in();

  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  // disable all VDP2 backgrounds (e.g: the Sega bios logo)
  vdp2.reg.BGON = 0;

  // zeroize BACK color
  vdp2.reg.BKTAU = 0;
  vdp2.reg.BKTAL = 0;
  vdp2.vram.u16[0] = 0;

  vdp2.reg.PRISA = PRISA__S0PRIN(1); // Sprite register 0 PRIority Number

  /* TVM settings must be performed from the second H-blank IN interrupt after the
  V-blank IN interrupt to the H-blank IN interrupt immediately after the V-blank
  OUT interrupt. */
  // "normal" display resolution, 16 bits per pixel, 512x256 framebuffer
  vdp1.reg.TVMR = TVMR__TVM__NORMAL;

  // swap framebuffers every 1 cycle; non-interlace
  vdp1.reg.FBCR = 0;

  // during a framebuffer erase cycle, write the color "black" to each pixel
  constexpr uint16_t black = 0x0000;
  vdp1.reg.EWDR = black;

  // erase upper-left coordinate
  vdp1.reg.EWLR = EWLR__16BPP_X1(0) | EWLR__Y1(0);

  // erase lower-right coordinate
  vdp1.reg.EWRR = EWRR__16BPP_X3(319) | EWRR__Y3(239);

  vdp1.vram.cmd[0].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__SYSTEM_CLIP_COORDINATES;
  vdp1.vram.cmd[0].LINK = 0;
  vdp1.vram.cmd[0].XC = 319;
  vdp1.vram.cmd[0].YC = 239;

  vdp1.vram.cmd[1].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__LOCAL_COORDINATE;
  vdp1.vram.cmd[1].LINK = 0;
  vdp1.vram.cmd[1].XA = 0;
  vdp1.vram.cmd[1].YA = 0;

  vdp1.vram.cmd[2].CTRL = CTRL__END;

  draw_state.cmd_ix = 2;

  // start drawing (execute the command list) on every frame
  vdp1.reg.PTMR = PTMR__PTM__FRAME_CHANGE;

  // free-running timer
  sh2.reg.TCR = TCR__CKS__INTERNAL_DIV128;
  sh2.reg.FTCSR = 0;

  // initialize smpc
  smpc.reg.DDR1 = 0; // INPUT
  smpc.reg.DDR2 = 0; // INPUT
  smpc.reg.IOSEL = 0; // SMPC control
  smpc.reg.EXLE = 0; //

  // interrupts
  sh2_vec[SCU_VEC__SMPC] = (u32)(&smpc_int);
  sh2_vec[SCU_VEC__V_BLANK_IN] = (u32)(&v_blank_in_int);

  scu.reg.IST = 0;
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);
}

extern "C"
void start(void)
{
  main();
  while (1);
}