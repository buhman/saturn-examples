#include <cstdio>
#include <cassert>
#include <cstring>
#include <cerrno>

#include "parse.hpp"
#include "strings.hpp"

static uint16_t _cent_to_fns[] = {
  0x0,
  0x3d,
  0x7d,
  0xc2,
  0x10a,
  0x157,
  0x1a8,
  0x1fe,
  0x25a,
  0x2ba,
  0x321,
  0x38d,
};

constexpr std::tuple<uint16_t, uint16_t>
midi_note_to_oct_fns(const int32_t midi_note)
{
  int32_t a440_note = midi_note - 69;
  uint16_t oct = (a440_note / 12) & 0xf;
  uint32_t cent = static_cast<uint32_t>(a440_note) % 12;
  uint16_t fns = _cent_to_fns[cent];
  return {oct, fns};
}

void
dump_midi(midi::midi_event_t& midi_event)
{
  switch (midi_event.type) {
  case midi::midi_event_t::type_t::note_on:
    {
      printf("      note_on %d\n", (int)midi_event.data.note_on.note);
      //auto&& [oct, fns] = midi_note_to_oct_fns(midi_event.data.note_on.note);
      //printf("      oct %d fns %d\n", oct, fns);
    }
    break;
  case midi::midi_event_t::type_t::note_off:
    {
      printf("      note_off %d\n", (int)midi_event.data.note_on.note);
    }
    break;
  default:
    break;
  }
}

int parse(uint8_t const * start)
{
  uint8_t const * buf = &start[0];
  auto header_o = midi::parse::header(buf);
  if (!header_o) {
    printf("invalid header\n");
    return -1;
  }
  midi::header_t header;
  std::tie(buf, header) = *header_o;

  printf("header.format: %s\n", midi::strings::header_format(header.format).c_str());
  printf("header.ntrks: %d\n", header.ntrks);

  for (int32_t i = 0; i < header.ntrks; i++) {
    printf("track[%d]:\n", i);

    auto track_o = midi::parse::track(buf);
    if (!track_o) {
      printf("invalid track\n");
      return -1;
    }

    midi::track_t track;
    std::tie(buf, track) = *track_o;

    printf("  track.length: %d\n", track.length);

    uint8_t const * track_start = buf;
    uint8_t running_status = 0;
    while (buf - track_start < track.length) {
      printf("  event:\n");
      auto mtrk_event_o = midi::parse::mtrk_event(buf, running_status);
      if (!mtrk_event_o) {
	printf("    invalid mtrk_event\n");
	printf("%02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);
	return -1;
      }

      midi::mtrk_event_t mtrk_event;
      std::tie(buf, mtrk_event) = *mtrk_event_o;
      printf("    delta_time: %d\n", mtrk_event.delta_time);
      switch (mtrk_event.event.type) {
      case midi::event_t::type_t::midi:
	printf("    midi:\n");
	dump_midi(mtrk_event.event.event.midi);
	break;
      case midi::event_t::type_t::sysex:
	printf("    sysex:\n");
	break;
      case midi::event_t::type_t::meta:
	{
	  printf("    meta:\n");
	  auto& meta = mtrk_event.event.event.meta;
	  printf("      type: %d\n", meta.type);
	  if (meta.type == 1) {
	    char str[meta.length + 1] = {0};
	    memcpy(str, meta.data, meta.length);
	    printf("      data: %s\n", meta.data);
	  }
	}
	break;
      default:
	assert(false);
      }
    }

    assert(buf - track_start == track.length);
    printf("trailing/unparsed data: %ld\n", track.length - (buf - track_start));
  }
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("argc < 2\n");
    return -1;
  }

  printf("%s\n", argv[1]);

  FILE * fp = fopen(argv[1], "rb");
  if (fp == nullptr) {
    printf("fopen: %s\n", strerror(errno));
    return -1;
  }
  int ret = fseek(fp, 0L, SEEK_END);
  assert(ret >= 0);
  int size = ftell(fp);
  assert(size >= 0);
  ret = fseek(fp, 0L, SEEK_SET);
  assert(ret >= 0);

  uint8_t start[size];
  size_t read_length = fread(start, size, 1, fp);
  assert(read_length == 1);

  parse(start);

  return 0;
}
