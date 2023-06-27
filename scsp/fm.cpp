#include <cstdint>
#include <optional>

#include "vdp2.h"
#include "smpc.h"
#include "scu.h"
#include "sh2.h"
#include "scsp.h"

#include "../common/copy.hpp"
#include "../common/intback.hpp"
#include "../common/vdp2_func.hpp"
#include "../common/string.hpp"

extern void * _sine_start __asm("_binary_scsp_sine_44100_s16be_1ch_100sample_pcm_start");
extern void * _sine_size __asm("_binary_scsp_sine_44100_s16be_1ch_100sample_pcm_size");

struct mask_bit {
  uint32_t mask;
  uint32_t bit;
};

template <typename T>
constexpr inline std::remove_volatile_t<typename T::reg_type> get(const typename T::reg_type r)
{
  return (r >> T::bit) & T::mask;
}

template <typename T>
constexpr inline void set(typename T::reg_type& r, uint32_t n)
{
  r = (r & ~(T::mask << T::bit)) | ((n & T::mask) << T::bit);
}

template <typename T>
constexpr inline void incdec(typename T::reg_type& r, int32_t n)
{
  int32_t ni = static_cast<int32_t>(get<T>(r)) + n;
  set<T>(r, ni);
}


#define BITS(N, T, M, B)			   \
  struct N { using reg_type = T;		   \
             static constexpr uint32_t mask = M;   \
             static constexpr uint32_t bit  = B; }

namespace loop {
  BITS(kyonex, reg16, 0b1, 12);
  BITS(kyonb,  reg16, 0b1, 11);
  BITS(sbctl,  reg16, 0b11, 9);
  BITS(ssctl,  reg16, 0b11, 7);
  BITS(lpctl,  reg16, 0b11, 5);
  BITS(pcm8b,  reg16, 0b1,  4);
}

namespace sa {
  BITS(sa, reg32, 0xf'ffff, 0);
}

namespace lsa {
  BITS(lsa, reg16, 0xffff, 0);
}

namespace lea {
  BITS(lea, reg16, 0xffff, 0);
}

namespace eg {
  BITS(d2r,    reg32, 0x1f, 11 + 16);
  BITS(d1r,    reg32, 0x1f, 6 + 16);
  BITS(eghold, reg32, 0b1,  5 + 16);
  BITS(ar,     reg32, 0x1f, 0 + 16);
  BITS(lpslnk, reg32, 0b1,  14);
  BITS(krs,    reg32, 0xf,  10);
  BITS(dl,     reg32, 0x1f, 5);
  BITS(rr,     reg32, 0x1f, 0);
}

namespace fm {
  BITS(stwinh, reg32, 0b1,  9 + 16);
  BITS(sdir,   reg32, 0b1,  8 + 16);
  BITS(tl,     reg32, 0xff, 0 + 16);
  BITS(mdl,    reg32, 0xf,  12);
  BITS(mdxsl,  reg32, 0x3f, 6);
  BITS(mdysl,  reg32, 0x3f, 0);
}

namespace pitch {
  BITS(oct, reg16, 0xf, 11);
  BITS(fns, reg16, 0x3ff, 0);
}

namespace lfo {
  BITS(lfore,  reg16, 0b1,   15);
  BITS(lfof,   reg16, 0x1f,  10);
  BITS(plfows, reg16, 0b11,  8);
  BITS(plfos,  reg16, 0b111, 5);
  BITS(alfows, reg16, 0b11,  3);
  BITS(alfos,  reg16, 0b111, 0);
}

namespace mixer {
  BITS(isel,  reg32, 0b1111,  3 + 16);
  BITS(imxl,  reg32, 0b111,   0 + 16);
  BITS(disdl, reg32, 0b111,   13);
  BITS(dipan, reg32, 0b11111, 8);
  BITS(efsdl, reg32, 0b111,   5);
  BITS(efpan, reg32, 0b11111, 0);
}

enum class scsp_name : uint8_t {
  kyonex,
  kyonb,
  sbctl,
  ssctl,
  lpctl,
  pcm8b,

  sa,

  lsa,

  lea,

  d2r,
  d1r,
  eghold,
  ar,
  lpslnk,
  krs,
  dl,
  rr,

  stwinh,
  sdir,
  tl,
  mdl,
  mdxsl,
  mdysl,

  oct,
  fns,

  lfore,
  lfof,
  plfows,
  plfos,
  alfows,
  alfos,

  isel,
  imxl,
  disdl,
  dipan,
  efsdl,
  efpan,

  FIRST = kyonex,
  LAST = efpan,
};

std::optional<scsp_name> grid[7][8] = {
  // 0     1      2      3  4      5   6     7
  {scsp_name::kyonex, scsp_name::kyonb, scsp_name::sbctl,  std::nullopt,     scsp_name::ssctl,  std::nullopt,      scsp_name::lpctl, scsp_name::pcm8b},
  {scsp_name::sa,     std::nullopt,     std::nullopt,      scsp_name::lsa,   std::nullopt,      std::nullopt,      scsp_name::lea,   std::nullopt},
  {scsp_name::d2r,    scsp_name::d1r,   scsp_name::eghold, scsp_name::ar,    scsp_name::lpslnk, scsp_name::krs,    scsp_name::dl,    scsp_name::rr},
  {scsp_name::stwinh, std::nullopt,     scsp_name::sdir,   scsp_name::tl,    scsp_name::mdl,    scsp_name::mdxsl,  std::nullopt,     scsp_name::mdysl},
  {scsp_name::oct,    std::nullopt,     std::nullopt,      std::nullopt,     scsp_name::fns,    std::nullopt,      std::nullopt,     std::nullopt},
  {scsp_name::lfore,  scsp_name::lfof,  scsp_name::plfows, std::nullopt,     scsp_name::plfos,  scsp_name::alfows, std::nullopt,     scsp_name::alfos},
  {scsp_name::isel,   scsp_name::imxl,  std::nullopt,      scsp_name::disdl, scsp_name::dipan, scsp_name::efsdl,  std::nullopt,     scsp_name::efpan},
};

uint32_t get_reg(scsp_slot& slot, scsp_name reg_name)
{
  switch (reg_name) {
  case scsp_name::kyonex: return get<loop::kyonex>(slot.LOOP); break;
  case scsp_name::kyonb:  return get<loop::kyonb >(slot.LOOP); break;
  case scsp_name::sbctl:  return get<loop::sbctl >(slot.LOOP); break;
  case scsp_name::ssctl:  return get<loop::ssctl >(slot.LOOP); break;
  case scsp_name::lpctl:  return get<loop::lpctl >(slot.LOOP); break;
  case scsp_name::pcm8b:  return get<loop::pcm8b >(slot.LOOP); break;

  case scsp_name::sa:  return get<sa::sa  >(slot.SA); break;

  case scsp_name::lsa: return get<lsa::lsa>(slot.LSA); break;

  case scsp_name::lea: return get<lea::lea>(slot.LEA); break;

  case scsp_name::d2r:    return get<eg::d2r   >(slot.EG); break;
  case scsp_name::d1r:    return get<eg::d1r   >(slot.EG); break;
  case scsp_name::eghold: return get<eg::eghold>(slot.EG); break;
  case scsp_name::ar:     return get<eg::ar    >(slot.EG); break;
  case scsp_name::lpslnk: return get<eg::lpslnk>(slot.EG); break;
  case scsp_name::krs:    return get<eg::krs   >(slot.EG); break;
  case scsp_name::dl:     return get<eg::dl    >(slot.EG); break;
  case scsp_name::rr:     return get<eg::rr    >(slot.EG); break;

  case scsp_name::stwinh: return get<fm::stwinh>(slot.FM); break;
  case scsp_name::sdir:   return get<fm::sdir  >(slot.FM); break;
  case scsp_name::tl:     return get<fm::tl    >(slot.FM); break;
  case scsp_name::mdl:    return get<fm::mdl   >(slot.FM); break;
  case scsp_name::mdxsl:  return get<fm::mdxsl >(slot.FM); break;
  case scsp_name::mdysl:  return get<fm::mdysl >(slot.FM); break;

  case scsp_name::oct: return get<pitch::oct>(slot.PITCH); break;
  case scsp_name::fns: return get<pitch::fns>(slot.PITCH); break;

  case scsp_name::lfore:  return get<lfo::lfore >(slot.LFO); break;
  case scsp_name::lfof:   return get<lfo::lfof  >(slot.LFO); break;
  case scsp_name::plfows: return get<lfo::plfows>(slot.LFO); break;
  case scsp_name::plfos:  return get<lfo::plfos >(slot.LFO); break;
  case scsp_name::alfows: return get<lfo::alfows>(slot.LFO); break;
  case scsp_name::alfos:  return get<lfo::alfos >(slot.LFO); break;

  case scsp_name::isel:  return get<mixer::isel >(slot.MIXER); break;
  case scsp_name::imxl:  return get<mixer::imxl >(slot.MIXER); break;
  case scsp_name::disdl: return get<mixer::disdl>(slot.MIXER); break;
  case scsp_name::dipan: return get<mixer::dipan>(slot.MIXER); break;
  case scsp_name::efsdl: return get<mixer::efsdl>(slot.MIXER); break;
  case scsp_name::efpan: return get<mixer::efpan>(slot.MIXER); break;
  }
  while (1) {}
}

void incdec_reg(scsp_slot& slot, scsp_name reg_name, int32_t dir)
{
  switch (reg_name) {
  case scsp_name::kyonex: return set<loop::kyonex>(slot.LOOP, 1); break;
  case scsp_name::kyonb:  return incdec<loop::kyonb >(slot.LOOP, dir); break;
  case scsp_name::sbctl:  return incdec<loop::sbctl >(slot.LOOP, dir); break;
  case scsp_name::ssctl:  return incdec<loop::ssctl >(slot.LOOP, dir); break;
  case scsp_name::lpctl:  return incdec<loop::lpctl >(slot.LOOP, dir); break;
  case scsp_name::pcm8b:  return incdec<loop::pcm8b >(slot.LOOP, dir); break;

  case scsp_name::sa:  return incdec<sa::sa  >(slot.SA, dir); break;

  case scsp_name::lsa: return incdec<lsa::lsa>(slot.LSA, dir); break;

  case scsp_name::lea: return incdec<lea::lea>(slot.LEA, dir); break;

  case scsp_name::d2r:    return incdec<eg::d2r   >(slot.EG, dir); break;
  case scsp_name::d1r:    return incdec<eg::d1r   >(slot.EG, dir); break;
  case scsp_name::eghold: return incdec<eg::eghold>(slot.EG, dir); break;
  case scsp_name::ar:     return incdec<eg::ar    >(slot.EG, dir); break;
  case scsp_name::lpslnk: return incdec<eg::lpslnk>(slot.EG, dir); break;
  case scsp_name::krs:    return incdec<eg::krs   >(slot.EG, dir); break;
  case scsp_name::dl:     return incdec<eg::dl    >(slot.EG, dir); break;
  case scsp_name::rr:     return incdec<eg::rr    >(slot.EG, dir); break;

  case scsp_name::stwinh: return incdec<fm::stwinh>(slot.FM, dir); break;
  case scsp_name::sdir:   return incdec<fm::sdir  >(slot.FM, dir); break;
  case scsp_name::tl:     return incdec<fm::tl    >(slot.FM, dir); break;
  case scsp_name::mdl:    return incdec<fm::mdl   >(slot.FM, dir); break;
  case scsp_name::mdxsl:  return incdec<fm::mdxsl >(slot.FM, dir); break;
  case scsp_name::mdysl:  return incdec<fm::mdysl >(slot.FM, dir); break;

  case scsp_name::oct: return incdec<pitch::oct>(slot.PITCH, dir); break;
  case scsp_name::fns: return incdec<pitch::fns>(slot.PITCH, dir); break;

  case scsp_name::lfore:  return incdec<lfo::lfore >(slot.LFO, dir); break;
  case scsp_name::lfof:   return incdec<lfo::lfof  >(slot.LFO, dir); break;
  case scsp_name::plfows: return incdec<lfo::plfows>(slot.LFO, dir); break;
  case scsp_name::plfos:  return incdec<lfo::plfos >(slot.LFO, dir); break;
  case scsp_name::alfows: return incdec<lfo::alfows>(slot.LFO, dir); break;
  case scsp_name::alfos:  return incdec<lfo::alfos >(slot.LFO, dir); break;

  case scsp_name::isel:  return incdec<mixer::isel >(slot.MIXER, dir); break;
  case scsp_name::imxl:  return incdec<mixer::imxl >(slot.MIXER, dir); break;
  case scsp_name::disdl: return incdec<mixer::disdl>(slot.MIXER, dir); break;
  case scsp_name::dipan: return incdec<mixer::dipan>(slot.MIXER, dir); break;
  case scsp_name::efsdl: return incdec<mixer::efsdl>(slot.MIXER, dir); break;
  case scsp_name::efpan: return incdec<mixer::efpan>(slot.MIXER, dir); break;
  }
  while (1) {}
}

struct label_value {
  uint8_t name[7];
  uint8_t len;
  struct {
    int8_t y;
    int8_t x;
  } label;
  struct {
    int8_t y;
    int8_t x;
  } value;
};

template <typename E>
constexpr typename std::underlying_type<E>::type u(E e) noexcept
{
  return static_cast<typename std::underlying_type<E>::type>(e);
}

constexpr inline int32_t clz(uint32_t v)
{
  if (v == 0) return 32;
  int32_t n = 0;
  if ((v & 0xFFFF0000) == 0) { n = n + 16; v = v << 16; }
  if ((v & 0xFF000000) == 0) { n = n +  8; v = v <<  8; }
  if ((v & 0xF0000000) == 0) { n = n +  4; v = v <<  4; }
  if ((v & 0xC0000000) == 0) { n = n +  2; v = v <<  2; }
  if ((v & 0x80000000) == 0) { n = n +  1; }
  return n;
}

constexpr inline int32_t hl(const uint32_t v)
{
  // return: number digits in v when represented as a base16 string
  int32_t n = 32 - clz(v);
  n = (n + 3) & ~0x03; // round up to nearest multiple of 4
  return n >> 2; // divide by 4
}

label_value label_value_table[] = {
  [u(scsp_name::kyonex)] = { "KX", hl(loop::kyonex::mask), {1, 2}, {2, 2} },
  [u(scsp_name::kyonb)] =  { "KB", hl(loop::kyonb::mask), {1, 7}, {2, 7} },
  [u(scsp_name::sbctl)] =  { "SBCTL", hl(loop::sbctl::mask), {1, 12}, {2, 12} },
  [u(scsp_name::ssctl)] =  { "SSCTL", hl(loop::ssctl::mask), {1, 20}, {2, 20} },
  [u(scsp_name::lpctl)] =  { "LPCTL", hl(loop::lpctl::mask), {1, 28}, {2, 28} },
  [u(scsp_name::pcm8b)] =  { "8B", hl(loop::pcm8b::mask), {1, 36}, {2, 36} },

  [u(scsp_name::sa)] =  { "SA", hl(sa::sa::mask), {4, 2}, {4, 5} },

  [u(scsp_name::lsa)] = { "LSA", hl(lsa::lsa::mask), {4, 15}, {4, 19} },

  [u(scsp_name::lea)] = { "LEA", hl(lea::lea::mask), {4, 28}, {4, 32} },

  [u(scsp_name::d2r)] =    { "D2R", hl(eg::d2r::mask), {6, 2}, {7, 2} },
  [u(scsp_name::d1r)] =    { "D1R", hl(eg::d1r::mask), {6, 7}, {7, 7} },
  [u(scsp_name::eghold)] = { "HO", hl(eg::eghold::mask), {6, 12}, {7, 12} },
  [u(scsp_name::ar)] =     { "AR", hl(eg::ar::mask), {6, 16}, {7, 16} },
  [u(scsp_name::lpslnk)] = { "LS", hl(eg::lpslnk::mask), {6, 20}, {7, 20} },
  [u(scsp_name::krs)] =    { "KRS", hl(eg::krs::mask), {6, 24}, {7, 24} },
  [u(scsp_name::dl)] =     { "DL", hl(eg::dl::mask), {6, 29}, {7, 29} },
  [u(scsp_name::rr)] =     { "RR", hl(eg::rr::mask), {6, 33}, {7, 33} },

  [u(scsp_name::stwinh)] = { "STWINH", hl(fm::stwinh::mask), {9, 2}, {10, 2} },
  [u(scsp_name::sdir)] =   { "SDIR", hl(fm::sdir::mask), {9, 10}, {10, 10} },
  [u(scsp_name::tl)] =     { "TL", hl(fm::tl::mask), {9, 16}, {10, 16} },
  [u(scsp_name::mdl)] =    { "MDL", hl(fm::mdl::mask), {9, 20}, {10, 20} },
  [u(scsp_name::mdxsl)] =  { "MDXSL", hl(fm::mdxsl::mask), {9, 25}, {10, 25} },
  [u(scsp_name::mdysl)] =  { "MDYSL", hl(fm::mdysl::mask), {9, 32}, {10, 32} },

  [u(scsp_name::oct)] = { "OCT", hl(pitch::oct::mask), {12, 11}, {12, 15} },
  [u(scsp_name::fns)] = { "FNS", hl(pitch::fns::mask), {12, 21}, {12, 25} },

  [u(scsp_name::lfore)] =  { "LFORE", hl(lfo::lfore::mask), {14, 2}, {15, 2} },
  [u(scsp_name::lfof)] =   { "LFOF", hl(lfo::lfof::mask), {14, 8}, {15, 8} },
  [u(scsp_name::plfows)] = { "PLFOWS", hl(lfo::plfows::mask), {14, 13}, {15, 13} },
  [u(scsp_name::plfos)] =  { "PLFOS", hl(lfo::plfos::mask), {14, 20}, {15, 20} },
  [u(scsp_name::alfows)] = { "ALFOWS", hl(lfo::alfows::mask), {14, 26}, {15, 26} },
  [u(scsp_name::alfos)] =  { "ALFOS", hl(lfo::alfos::mask), {14, 33}, {15, 33} },

  [u(scsp_name::isel)] =  { "ISEL", hl(mixer::isel::mask), {17, 2}, {18, 2} },
  [u(scsp_name::imxl)] =  { "IMXL", hl(mixer::imxl::mask), {17, 8}, {18, 8} },
  [u(scsp_name::disdl)] = { "DISDL", hl(mixer::disdl::mask), {17, 14}, {18, 14} },
  [u(scsp_name::dipan)] = { "DIPAN", hl(mixer::dipan::mask), {17, 20}, {18, 20} },
  [u(scsp_name::efsdl)] = { "EFSDL", hl(mixer::efsdl::mask), {17, 26}, {18, 26} },
  [u(scsp_name::efpan)] = { "EFPAN", hl(mixer::efpan::mask), {17, 32}, {18, 32} },
};

extern void * _nec_bitmap_start __asm("_binary_res_nec_bitmap_bin_start");

constexpr inline uint16_t rgb15(int32_t r, int32_t g, int32_t b)
{
  return ((b & 31) << 10) | ((g & 31) << 5) | ((r & 31) << 0);
}

void palette_data()
{
  vdp2.cram.u16[1 + 0 ] = rgb15( 0,  0,  0);
  vdp2.cram.u16[2 + 0 ] = rgb15(31, 31, 31);

  vdp2.cram.u16[1 + 16] = rgb15(31, 31, 31);
  vdp2.cram.u16[2 + 16] = rgb15( 0,  0,  0);

  vdp2.cram.u16[1 + 32] = rgb15(10, 10, 10);
  vdp2.cram.u16[2 + 32] = rgb15(31, 31, 31);
}

namespace pix_fmt_4bpp
{
  constexpr inline uint32_t
  bit(uint8_t n, int32_t i)
  {
    i &= 7;
    auto b = (n >> (7 - i)) & 1;
    return ((b + 1) << ((7 - i) * 4));
  }

  constexpr inline uint32_t
  bits(uint8_t n)
  {
    return
        bit(n, 0) | bit(n, 1) | bit(n, 2) | bit(n, 3)
      | bit(n, 4) | bit(n, 5) | bit(n, 6) | bit(n, 7);
  }

  static_assert(bits(0b1100'1110) == 0x2211'2221);
  static_assert(bits(0b1010'0101) == 0x2121'1212);
  static_assert(bits(0b1000'0000) == 0x2111'1111);
}

void cell_data()
{
  const uint8_t * normal = reinterpret_cast<uint8_t*>(&_nec_bitmap_start);

  for (int ix = 0; ix <= (0x7f - 0x20); ix++) {
    for (int y = 0; y < 8; y++) {
      const uint8_t row_n = normal[ix * 8 + y];
      vdp2.vram.u32[ 0 + (ix * 8) + y] = pix_fmt_4bpp::bits(row_n);
    }
  }
}

struct count_flop {
  s8 count;
  u8 flop;
  u8 das;
  u8 repeat;
};

struct input {
  count_flop right;
  count_flop left;
  count_flop down;
  count_flop up;
  count_flop start;
  count_flop a;
  count_flop b;
  count_flop c;
  count_flop r;
  count_flop x;
  count_flop y;
  count_flop z;
  count_flop l;
};

constexpr int input_arr = 10;
constexpr int input_das = 20;
constexpr int input_debounce = 2;

static inline void
input_count(count_flop& button, uint32_t input, uint32_t mask)
{
  if ((input & mask) == 0) {
    if (button.count < input_debounce)
      button.count += 1;
    else
      button.das += 1;
  } else {
    if (button.count == 0) {
      button.flop = 0;
      button.das = 0;
      button.repeat = 0;
    }
    else if (button.count > 0)
      button.count -= 1;
  }
}

static inline int32_t
input_flopped(count_flop& button)
{
  if (button.count == input_debounce && button.flop == 0) {
    button.flop = 1;
    return 1;
  } else if (button.flop == 1 && button.das == input_das && button.repeat == 0) {
    button.repeat = 1;
    button.das = 0;
    return 2;
  } else if (button.repeat == 1 && (button.das == input_arr)) {
    button.das = 0;
    return 2;
  } else {
    return 0;
  }
}

struct cursor {
  int8_t x;
  int8_t y;

  void left()  { do { x = (x - 1) & 7; } while (!grid[y][x]); }
  void right() { do { x = (x + 1) & 7; } while (!grid[y][x]); }
  void up()
  {
    y -= 1;
    if (y < 0) y = 6;
    while (!grid[y][x]) { x--; };
  }
  void down()
  {
    y += 1;
    if (y > 6) y = 0;
    while (!grid[y][x]) { x--; };
  }
};

struct state {
  uint8_t slot_ix;
  struct input input;
  struct cursor cursor;
};

static struct state state = { 0 };

void digital_callback(uint8_t fsm_state, uint8_t data)
{
  switch (fsm_state) {
  case intback::DATA1:
    input_count(state.input.right, data, DIGITAL__1__RIGHT);
    input_count(state.input.left, data, DIGITAL__1__LEFT);
    input_count(state.input.down, data, DIGITAL__1__DOWN);
    input_count(state.input.up, data, DIGITAL__1__UP);
    input_count(state.input.start, data, DIGITAL__1__START);
    input_count(state.input.a, data, DIGITAL__1__A);
    input_count(state.input.c, data, DIGITAL__1__C);
    input_count(state.input.b, data, DIGITAL__1__B);
    break;
  case intback::DATA2:
    input_count(state.input.r, data, DIGITAL__2__R);
    input_count(state.input.x, data, DIGITAL__2__X);
    input_count(state.input.y, data, DIGITAL__2__Y);
    input_count(state.input.z, data, DIGITAL__2__Z);
    input_count(state.input.l, data, DIGITAL__2__L);
    break;
  default: break;
  }
}

extern "C"
void smpc_int(void) __attribute__ ((interrupt_handler));
void smpc_int(void)
{
  scu.reg.IST &= ~(IST__SMPC);
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);

  intback::fsm(digital_callback, nullptr);
}

constexpr int32_t plane_a = 2;
constexpr inline int32_t plane_offset(int32_t n) { return n * 0x2000; }

constexpr int32_t page_size = 64 * 64 * 2; // N0PNB__1WORD (16-bit)
constexpr int32_t plane_size = page_size * 1;

constexpr int32_t cell_size = (8 * 8) / 2; // N0CHCN__16_COLOR (4-bit)
constexpr int32_t character_size = cell_size * (1 * 1); // N0CHSZ__1x1_CELL
constexpr int32_t page_width = 64;

static int plane_ix = 0;

inline void
set_char(int32_t x, int32_t y, uint8_t palette, uint8_t c)
{
  const auto ix = (plane_offset(plane_a + plane_ix) / 2) + (y * page_width) + x;
  vdp2.vram.u16[ix] =
      PATTERN_NAME_TABLE_1WORD__PALETTE(palette)
    | PATTERN_NAME_TABLE_1WORD__CHARACTER((c - 0x20));
}

void render()
{
  //                 012345678901234
  uint8_t label[] = "scsp.reg.slot[  ]";
  string::hex(&label[14], 2, state.slot_ix);
  for (uint32_t i = 0; label[i] != 0; i++) {
    set_char(0 + i, 1, 0, label[i]);
  }

  constexpr int32_t y_offset = 3;

  for (uint32_t f_ix = u(scsp_name::FIRST); f_ix <= u(scsp_name::LAST); f_ix++) {
    label_value& lv = label_value_table[f_ix];
    for (uint32_t i = 0; lv.name[i] != 0; i++) {
      set_char(lv.label.x + i, lv.label.y + y_offset, 0, lv.name[i]);
    }

    uint8_t buf[lv.len];
    scsp_name name = static_cast<scsp_name>(f_ix);
    uint32_t value = get_reg(scsp.reg.slot[state.slot_ix], name);
    string::hex(buf, lv.len, value);
    bool selected = !(!grid[state.cursor.y][state.cursor.x]) && name == *grid[state.cursor.y][state.cursor.x];
    uint32_t palette = selected ? 1 : 0;
    for (uint32_t i = 0; i < lv.len; i++) {
      set_char(lv.value.x + i, lv.value.y + y_offset, palette, buf[i]);
    }
  }
}

namespace event {
  inline bool prev_slot() { return input_flopped(state.input.l) == 1; }
  inline bool next_slot() { return input_flopped(state.input.r) == 1; }
  inline bool cursor_left()  { return input_flopped(state.input.left ) >= 1; }
  inline bool cursor_right() { return input_flopped(state.input.right) >= 1; }
  inline bool cursor_up()    { return input_flopped(state.input.up   ) >= 1; }
  inline bool cursor_down()  { return input_flopped(state.input.down ) >= 1; }
  inline bool cursor_dec()   { return input_flopped(state.input.x ) >= 1; }
  inline bool cursor_inc()   { return input_flopped(state.input.y ) >= 1; }
}

void update()
{
  if (event::prev_slot())
    state.slot_ix = (state.slot_ix - 1) & 31;
  if (event::next_slot())
    state.slot_ix = (state.slot_ix + 1) & 31;
  if (event::cursor_left())  state.cursor.left();
  if (event::cursor_right()) state.cursor.right();
  if (event::cursor_up())    state.cursor.up();
  if (event::cursor_down())  state.cursor.down();
  if (grid[state.cursor.y][state.cursor.x]) {
    if (event::cursor_inc())   incdec_reg(scsp.reg.slot[state.slot_ix], *grid[state.cursor.y][state.cursor.x], +1);
    if (event::cursor_dec())   incdec_reg(scsp.reg.slot[state.slot_ix], *grid[state.cursor.y][state.cursor.x], -1);
  }
}

extern "C"
void v_blank_in_int(void) __attribute__ ((interrupt_handler));
void v_blank_in_int()
{
  scu.reg.IST &= ~(IST__V_BLANK_IN);
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);

  // flip planes;
  vdp2.reg.MPABN0 = MPABN0__N0MPB(0) | MPABN0__N0MPA(plane_a + plane_ix);
  plane_ix = !plane_ix;

  // wait at least 300us, as specified in the SMPC manual.
  // It appears reading FRC.H is mandatory and *must* occur before FRC.L on real
  // hardware.
  while ((sh2.reg.FTCSR & FTCSR__OVF) == 0 && sh2.reg.FRC.H == 0 && sh2.reg.FRC.L < 63);

  if ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) != 0) {
    // on real hardware, SF contains uninitialized garbage bits other than the
    // lsb.
    while ((smpc.reg.SF & 1) != 0);

    smpc.reg.SF = 0;

    smpc.reg.IREG[0].val = INTBACK__IREG0__STATUS_DISABLE;
    smpc.reg.IREG[1].val = ( INTBACK__IREG1__PERIPHERAL_DATA_ENABLE
                           | INTBACK__IREG1__PORT2_15BYTE
                           | INTBACK__IREG1__PORT1_15BYTE
                           );
    smpc.reg.IREG[2].val = INTBACK__IREG2__MAGIC;

    smpc.reg.COMREG = COMREG__INTBACK;
  }

  update();
  render();
}

void init_slots()
{
  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDOFF;
  while (smpc.reg.OREG[31].val != OREG31__SNDOFF);

  scsp.reg.ctrl.MIXER = MIXER__MEM4MB | MIXER__MVOL(0);

  for (long i = 0; i < 3200; i++) { asm volatile ("nop"); }   // wait for (way) more than 30µs
  
  /*
    The Saturn BIOS does not (un)initialize the DSP. Without zeroizing the DSP
    program, the SCSP DSP appears to have a program that continuously writes to
    0x30000 through 0x3ffff in sound RAM, which has the effect of destroying any
    samples stored there.
  */
  reg32 * dsp_steps = reinterpret_cast<reg32*>(&(scsp.reg.dsp.STEP[0].MPRO[0]));
  fill<reg32>(dsp_steps, 0, (sizeof (scsp.reg.dsp.STEP)));

  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDON;
  while (smpc.reg.OREG[31].val != OREG31__SNDON);

  for (long i = 0; i < 3200; i++) { asm volatile ("nop"); }   // wait for (way) more than 30µs

  const uint32_t * buf = reinterpret_cast<uint32_t*>(&_sine_start);
  const uint32_t size = reinterpret_cast<uint32_t>(&_sine_size);
  copy<uint32_t>(&scsp.ram.u32[0], buf, size);

  for (int i = 0; i < 32; i++) {
    scsp_slot& slot = scsp.reg.slot[i];
    // start address (bytes)
    slot.SA = SA__LPCTL__NORMAL | SA__SA(0); // kx kb sbctl[1:0] ssctl[1:0] lpctl[1:0] 8b sa[19:0]
    slot.LSA = 0; // loop start address (samples)
    slot.LEA = 100; // loop end address (samples)
    slot.EG = EG__AR(0x1f) | EG__RR(0x1f); // d2r d1r ho ar krs dl rr
    slot.FM = 0; // stwinh sdir tl mdl mdxsl mdysl
    //slot.PITCH = PITCH__OCT(-1) | PITCH__FNS(0xc2); // oct fns
    slot.PITCH = 0x78c2;
    slot.LFO = 0; // lfof plfows
    slot.MIXER = MIXER__DISDL(0b101); // disdl dipan efsdl efpan
  }

  scsp.reg.ctrl.MIXER = MIXER__MEM4MB | MIXER__MVOL(0xf);
}

void main()
{
  init_slots();

  v_blank_in();

  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  /* set the color mode to 5bits per channel, 1024 colors */
  vdp2.reg.RAMCTL = RAMCTL__CRMD__RGB_5BIT_1024;

  /* enable display of NBG0 */
  vdp2.reg.BGON = BGON__N0ON;

  /* set character format for NBG0 to palettized 16 color
     set enable "cell format" for NBG0
     set character size for NBG0 to 1x1 cell */
  vdp2.reg.CHCTLA = CHCTLA__N0CHCN__16_COLOR
                  | CHCTLA__N0BMEN__CELL_FORMAT
                  | CHCTLA__N0CHSZ__1x1_CELL;
  /* "Note: In color RAM modes 0 and 2, 2048-color becomes 1024-color" */

  /* use 1-word (16-bit) pattern names */
  vdp2.reg.PNCN0 = PNCN0__N0PNB__1WORD;

  /* plane size */
  vdp2.reg.PLSZ = PLSZ__N0PLSZ__1x1;

  /* map plane offset
     1-word: value of bit 6-0 * 0x2000
     2-word: value of bit 5-0 * 0x4000
  */
  vdp2.reg.MPOFN = MPOFN__N0MP(0); // bits 8~6
  vdp2.reg.MPABN0 = MPABN0__N0MPB(0) | MPABN0__N0MPA(plane_a); // bits 5~0
  vdp2.reg.MPCDN0 = MPABN0__N0MPD(0) | MPABN0__N0MPC(0); // bits 5~0

  // zeroize character/cell data from 0 up to plane_a_offset
  fill<uint32_t>(&vdp2.vram.u32[(0 / 4)], 0, plane_offset(plane_a));

  // zeroize plane_a; `0` is the ascii 0x20 ("space") which doubles as
  // "transparency" character.
  fill<uint32_t>(&vdp2.vram.u32[(plane_offset(plane_a) / 4)], 0, plane_size * 2);

  palette_data();
  cell_data();

  // free-running timer
  sh2.reg.TCR = TCR__CKS__INTERNAL_DIV128;
  sh2.reg.FTCSR = 0;

  // initialize smpc
  smpc.reg.DDR1 = 0; // INPUT
  smpc.reg.DDR2 = 0; // INPUT
  smpc.reg.IOSEL = 0; // SMPC control
  smpc.reg.EXLE = 0; //

  sh2_vec[SCU_VEC__SMPC] = (u32)(&smpc_int);
  sh2_vec[SCU_VEC__V_BLANK_IN] = (u32)(&v_blank_in_int);

  scu.reg.IST = 0;
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);
}
