#include <cstdint>
#include <cassert>
#include <iostream>

#include "midi.hpp"
#include "generate.cpp"

namespace midi {
namespace test {

struct buf_n_t {
  uint32_t n;
  uint8_t buf[4];
  int32_t len;
};

void int_variable_length()
{
  static buf_n_t tests[] = {
    {0x0000'0000, {0x00}, 1},
    {0x0000'0040, {0x40}, 1},
    {0x0000'007f, {0x7f}, 1},

    {0x0000'0080, {0x81, 0x00}, 2},
    {0x0000'2000, {0xc0, 0x00}, 2},
    {0x0000'3fff, {0xff, 0x7f}, 2},

    {0x0000'4000, {0x81, 0x80, 0x00}, 3},
    {0x0010'0000, {0xc0, 0x80, 0x00}, 3},
    {0x001f'ffff, {0xff, 0xff, 0x7f}, 3},

    {0x0020'0000, {0x81, 0x80, 0x80, 0x00}, 4},
    {0x0800'0000, {0xc0, 0x80, 0x80, 0x00}, 4},
    {0x0fff'ffff, {0xff, 0xff, 0xff, 0x7f}, 4},
  };

  for (int i = 0; i < 12; i++) {
    uint8_t buf1[4];
    buf_n_t& test = tests[i];
    uint8_t * buf2 = generate::int_variable_length(&buf1[0], test.n);
    assert(buf2 - buf1 == test.len);
    for (int j = 0; j < test.len; j++)
      assert(buf1[j] == test.buf[j]);
  }
}

void header()
{
  header_t header = {
    .format = header_t::format_t::_2,
    .ntrks = 1,
    .division = {
      .type = division_t::type_t::time_code,
      .time_code = {
	.smpte = -30,
	.ticks_per_frame = 0x50
      }
    }
  };

  uint8_t buf[64] = {0xee};
  uint8_t * res = generate::header(buf, header);
  uint8_t expect[] = {
    0x4d, 0x54, 0x68, 0x64, // chunk type
    0x00, 0x00, 0x00, 0x06, // length
    0x00, 0x02, // format
    0x00, 0x01, // ntrks
    0xe2, 0x50, // division
  };
  assert(res - buf == 14);
  for (int i = 0; i < 14; i++)
    assert(expect[i] == buf[i]);
}

}
}

int main()
{
  using namespace midi::test;

  int_variable_length();
  header();

  return 0;
}
