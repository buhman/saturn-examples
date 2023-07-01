#include <tuple>
#include <optional>
#include <cstdint>
#include <bit>

#include "parse.hpp"

namespace midi {
namespace parse {

constexpr inline std::optional<std::tuple<buf_t, uint32_t>>
int_variable_length(buf_t buf)
{
  uint32_t n = 0;
  int32_t i = 0;
  while (i < 4) {
    n <<= 7;
    uint8_t b = buf[i++];
    n |= (b & 0x7f);
    if ((b & 0x80) == 0)
      return {{buf + i, n}};
  }

  return std::nullopt;
}

// note: there are no alignment requirements for the location of a
// fixed-length integer inside a midi file--reinterpret_cast+byteswap
// would be faster if it weren't that this is possibly-unaligned
// access.

constexpr inline std::tuple<buf_t, uint16_t>
int_fixed_length16(buf_t buf)
{
  uint16_t n = (buf[0] << 8 | buf[1] << 0);
  return {buf + (sizeof (uint16_t)), n};
}

constexpr inline std::tuple<buf_t, uint32_t>
int_fixed_length32(buf_t buf)
{
  uint32_t n = (buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3] << 0);
  return {buf + (sizeof (uint32_t)), n};
}

constexpr inline std::optional<buf_t>
header_chunk_type(buf_t buf)
{
  if (   buf[0] == 'M'
      && buf[1] == 'T'
      && buf[2] == 'h'
      && buf[3] == 'd')
    return {buf + 4};
  else
    return std::nullopt;
}

constexpr inline std::tuple<buf_t, division_t>
division(buf_t buf)
{
  uint16_t n;
  std::tie(buf, n) = int_fixed_length16(buf);
  if ((n & (1 << 15)) != 0) {
    int8_t smpte = (n >> 8);
    uint8_t ticks_per_frame = n & 0xff;
    return { buf, { .type = division_t::type_t::time_code,
		    .time_code = { smpte, ticks_per_frame }
                  } };
  } else {
    return { buf, { .type = division_t::type_t::metrical,
		    .metrical = { n }
                  } };
  }
}

std::optional<std::tuple<buf_t, header_t>>
header(buf_t buf)
{
  auto buf_o = header_chunk_type(buf);
  if (!buf_o) return std::nullopt;
  buf = *buf_o;
  uint32_t header_length;
  std::tie(buf, header_length) = int_fixed_length32(buf);
  if (header_length != 6) return std::nullopt;
  uint16_t format_num;
  std::tie(buf, format_num) = int_fixed_length16(buf);
  header_t::format_t format;
  switch (format_num) {
  case 0: format = header_t::format_t::_0; break;
  case 1: format = header_t::format_t::_1; break;
  case 2: format = header_t::format_t::_2; break;
  default: return std::nullopt;
  }
  uint16_t ntrks;
  std::tie(buf, ntrks) = int_fixed_length16(buf);
  division_t division;
  std::tie(buf, division) = parse::division(buf);
  return {{buf, {format, ntrks, division}}};
}

constexpr inline std::optional<buf_t>
track_chunk_type(buf_t buf)
{
  if (   buf[0] == 'M'
      && buf[1] == 'T'
      && buf[2] == 'r'
      && buf[3] == 'k')
    return {buf + 4};
  else
    return std::nullopt;
}

std::optional<std::tuple<buf_t, track_t>>
track(buf_t buf)
{
  auto buf_o = track_chunk_type(buf);
  if (!buf_o) return std::nullopt;
  buf = *buf_o;

  uint32_t length;
  std::tie(buf, length) = int_fixed_length32(buf);

  return {{buf, {length}}};
}

constexpr inline std::optional<midi_event_t::type_t>
midi_event_type(uint8_t status, uint8_t param0)
{
  uint8_t n = status & 0xf0;
  // do not increment buf; the caller needs to parse channel
  switch (n) {
  case 0x80: return {midi_event_t::type_t::note_off};
  case 0x90: return {midi_event_t::type_t::note_on};
  case 0xa0: return {midi_event_t::type_t::polyphonic_key_pressure};
  case 0xb0:
    if (param0 >= 121 && param0 <= 127)
      return {midi_event_t::type_t::channel_mode};
    else
      return {midi_event_t::type_t::control_change};
  case 0xc0: return {midi_event_t::type_t::program_change};
  case 0xd0: return {midi_event_t::type_t::channel_pressure};
  case 0xe0: return {midi_event_t::type_t::pitch_bend_change};
  default: return std::nullopt;
  };
}

constexpr inline int32_t
midi_event_message_length(midi_event_t::type_t type)
{
  if (type == midi_event_t::type_t::program_change || type == midi_event_t::type_t::channel_pressure)
    return 1;
  else
    return 2;
}

constexpr inline std::optional<std::tuple<buf_t, midi_event_t>>
midi_event(buf_t buf, uint8_t& running_status)
{
  if ((buf[0] & 0x80) == 0) {
    // invalid running_status
    if ((running_status & 0x80) == 0) return std::nullopt;
  } else {
    // this is a status change
    running_status = buf[0];
    buf++;
  }

  midi_event_t event;
  auto type_o = midi_event_type(running_status, buf[0]);
  if (!type_o) return std::nullopt;
  event.type = *type_o;
  event.data.note_off.channel = running_status & 0x0f;

  // it doesn't matter which union is used, as long as it is one with
  // 3 items. note_off is used throughout, even though it could
  // represent a type other than note_off.
  event.data.note_off.note = buf[0];
  // possibly-initializing the third field on a 2-field midi_event
  // does not matter, as the caller should ignore it anyway. This
  // harmless extraneous initialization means a branch is not needed.
  event.data.note_off.velocity = buf[1];
  buf += midi_event_message_length(event.type);

  // this does not validate that buf[1]/buf[2] are <= 0x7f
  return {{buf, event}};
}

constexpr inline std::optional<std::tuple<buf_t, sysex_event_t>>
sysex_event(buf_t buf)
{
  // not implemented
  return std::nullopt;
  /*
  if (buf[0] != 0xf0 && buf[0] != 0xf7) return std::nullopt;
  buf++;
  auto length_o = int_variable_length(buf);
  if (!length_o) return std::nullopt;
  uint32_t length;
  std::tie(buf, length) = *length_o;
  return {{buf + length, {buf, length}}};
  */
}

constexpr inline std::optional<std::tuple<buf_t, meta_event_t>>
meta_event(buf_t buf)
{
  if (buf[0] != 0xff) return std::nullopt;
  uint8_t type = buf[1];
  buf += 2;
  auto length_o = int_variable_length(buf);
  if (!length_o) return std::nullopt;
  uint32_t length;
  std::tie(buf, length) = *length_o;
  return {{buf + length, {buf, length, type}}};
}

constexpr inline std::optional<std::tuple<buf_t, event_t>>
event(buf_t buf, uint8_t& running_status)
{
  if        (auto midi_o  = midi_event(buf, running_status)) {
    midi_event_t midi;
    std::tie(buf, midi) = *midi_o;
    return {{buf, {event_t::type_t::midi, {.midi = midi}}}};
  } else if (auto meta_o  = meta_event(buf)) {
    running_status = 0;
    meta_event_t meta;
    std::tie(buf, meta) = *meta_o;
    return {{buf, {event_t::type_t::meta, {.meta = meta}}}};
  } else if (auto sysex_o = sysex_event(buf)) {
    running_status = 0;
    sysex_event_t sysex;
    std::tie(buf, sysex) = *sysex_o;
    return {{buf, {event_t::type_t::sysex, {.sysex = sysex}}}};
  } else {
    return std::nullopt;
  }
}

std::optional<std::tuple<buf_t, mtrk_event_t>>
mtrk_event(buf_t buf, uint8_t& running_status)
{
  auto delta_time_o = int_variable_length(buf);
  if (!delta_time_o) return std::nullopt;
  uint32_t delta_time;
  std::tie(buf, delta_time) = *delta_time_o;

  auto event_o = event(buf, running_status);
  if (!event_o) return std::nullopt;
  event_t event;
  std::tie(buf, event) = *event_o;
  return {{buf, {delta_time, event}}};
}

} // namespace parse
} // namespace midi
