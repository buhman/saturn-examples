#include <cstdint>
#include <iostream>
#include <cassert>

#include "midi.hpp"
#include "generate.hpp"

namespace midi {
namespace generate {

constexpr inline buf_t
int_variable_length(buf_t buf, const uint32_t n) noexcept
{
  int nonzero = 0;
  for (int i = 3; i > 0; i--) {
    uint8_t nib = (n >> (7 * i)) & 0x7f;
    if (nib != 0) nonzero = 1;
    if (nonzero) {
      buf[0] = 0x80 | nib;
      buf++;
    }
  }
  buf[0] = n & 0x7f;
  buf++;
  return buf;
}

// note: there are no alignment requirements for the location of a
// fixed-length integer inside a midi file--reinterpret_cast+byteswap
// would be faster if it weren't that this is possibly-unaligned
// access.

constexpr inline buf_t
int_fixed_length16(buf_t buf, const uint16_t n) noexcept
{
  buf[0] = (n >> 8) & 0xff;
  buf[1] = (n >> 0) & 0xff;
  return buf + (sizeof (uint16_t));
}

constexpr inline buf_t
int_fixed_length32(buf_t buf, const uint32_t n) noexcept
{
  buf[0] = (n >> 24) & 0xff;
  buf[1] = (n >> 16) & 0xff;
  buf[2] = (n >> 8 ) & 0xff;
  buf[3] = (n >> 0 ) & 0xff;
  return buf + (sizeof (uint32_t));
}

constexpr inline buf_t
header_chunk_type(buf_t buf) noexcept
{
  buf[0] = 'M';
  buf[1] = 'T';
  buf[2] = 'h';
  buf[3] = 'd';
  return buf + 4;
}

constexpr inline buf_t
division(buf_t buf, const division_t& division) noexcept
{
  if (division.type == division_t::type_t::metrical) {
    return int_fixed_length16(buf, division.metrical.ticks_per_quarter_note & 0x7fff);
  } else { // time_code
    buf[0] = division.time_code.smpte | 0x80;
    buf[1] = division.time_code.ticks_per_frame;
    return buf + 2;
  }
}

buf_t
header(buf_t buf, const header_t& header) noexcept
{
  buf = header_chunk_type(buf);
  buf = int_fixed_length32(buf, 6);

  uint16_t format;
  switch (header.format) {
  case header_t::format_t::_0: format = 0; break;
  case header_t::format_t::_1: format = 1; break;
  case header_t::format_t::_2: format = 2; break;
  default: assert(false);
  }
  buf = int_fixed_length16(buf, format);

  buf = int_fixed_length16(buf, header.ntrks);

  buf = division(buf, header.division);

  return buf;
}


constexpr inline buf_t
track_chunk_type(buf_t buf) noexcept
{
  buf[0] = 'M';
  buf[1] = 'T';
  buf[2] = 'r';
  buf[3] = 'k';
  return buf + 4;
}

buf_t
track(buf_t buf, const track_t& track) noexcept
{
  buf = track_chunk_type(buf);
  buf = int_fixed_length32(buf, track.length);
  return buf;
}

constexpr inline uint8_t
midi_status_byte(const midi_event_t::type_t midi_event_type) noexcept
{
  switch (midi_event_type) {
  case midi_event_t::type_t::note_off: return 0x80;
  case midi_event_t::type_t::note_on: return 0x90;
  case midi_event_t::type_t::polyphonic_key_pressure: return 0xa0;
  case midi_event_t::type_t::channel_mode: return 0xb0;
  case midi_event_t::type_t::control_change: return 0xb0;
  case midi_event_t::type_t::program_change: return 0xc0;
  case midi_event_t::type_t::channel_pressure: return 0xd0;
  case midi_event_t::type_t::pitch_bend_change: return 0xe0;
  default: assert(false);
  };
}

constexpr inline int32_t
midi_event_message_length(const midi_event_t::type_t type) noexcept
{
  if (type == midi_event_t::type_t::program_change || type == midi_event_t::type_t::channel_pressure)
    return 1;
  else
    return 2;
}

constexpr inline buf_t
midi_event(buf_t buf, const midi_event_t& event, uint8_t& running_status) noexcept
{
  uint8_t status = midi_status_byte(event.type) | event.data.note_off.channel;
  if (running_status != status) {
    running_status = status;
    buf[0] = status;
    buf++;
  }

  // it doesn't matter which union is used, as long as it is one with
  // 3 items. note_off is used throughout, even though it could
  // represent a type other than note_off.

  buf[0] = event.data.note_off.note;
  // possibly-initializing the third field on a 2-field midi_event
  // does not matter, as the caller should ignore it anyway. This
  // harmless extraneous initialization means a branch is not needed.
  buf[1] = event.data.note_off.velocity;
  return buf + midi_event_message_length(event.type);
}

constexpr inline buf_t
sysex_event(buf_t buf, const sysex_event_t& event) noexcept
{
  if not consteval {
    assert(false); // not implemented
  }
  return buf;
}

constexpr inline buf_t
meta_event(buf_t buf, const meta_event_t& meta) noexcept
{
  buf[0] = 0xff;
  buf[1] = meta.type;
  buf = int_variable_length(buf + 2, meta.length);
  for (uint32_t i = 0; i < meta.length; i++) {
    buf[i] = meta.data[i];
  }

  return buf + meta.length;
}

constexpr inline buf_t
event(buf_t buf, const event_t& event, uint8_t& running_status) noexcept
{
  switch (event.type) {
  case event_t::type_t::midi:
    return midi_event(buf, event.event.midi, running_status);
  case event_t::type_t::sysex:
    running_status = 0;
    return sysex_event(buf, event.event.sysex);
  case event_t::type_t::meta:
    running_status = 0;
    return meta_event(buf, event.event.meta);
  default: assert(false);
  }
}

buf_t
mtrk_event(buf_t buf, const mtrk_event_t& _event, uint8_t& running_status) noexcept
{
  buf = int_variable_length(buf, _event.delta_time);
  buf = event(buf, _event.event, running_status);
  return buf;
}

}
}
