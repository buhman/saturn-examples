#include <iostream>
#include <fstream>
#include <tuple>
#include <cassert>

#include "parse.hpp"
#include "strings.hpp"

int parse(uint8_t const * start)
{
  uint8_t const * buf = &start[0];
  auto header_o = midi::parse::header(buf);
  if (!header_o) {
    std::cerr << "invalid header\n";
    return -1;
  }
  midi::header_t header;
  std::tie(buf, header) = *header_o;

  std::cout << "header.format: " << midi::strings::header_format(header.format) << '\n';
  std::cout << "header.ntrks: " << header.ntrks << '\n';

  //while header.n
  // while header.ntrks:
     //
     // for event in events:
     //    ev

  for (int32_t i = 0; i < header.ntrks; i++) {
    std::cout << "track[" << i << "]:\n";

    auto track_o = midi::parse::track(buf);
    if (!track_o) {
      std::cerr << "invalid track\n";
      return -1;
    }

    uint32_t track_length;
    std::tie(buf, track_length) = *track_o;

    std::cout << "  track_length: " << track_length << '\n';

    uint8_t const * track_start = buf;
    while (buf - track_start < track_length) {
      std::cout << "  event:\n";
      auto mtrk_event_o = midi::parse::mtrk_event(buf);
      if (!mtrk_event_o) {
	std::cout << "    invalid mtrk_event\n";
	std::cout << std::hex << buf[0] << ' ' << buf[1] << ' ' << buf[2] << ' ' << buf[3];
	return -1;
      }

      midi::mtrk_event_t mtrk_event;
      std::tie(buf, mtrk_event) = *mtrk_event_o;
      std::cout << "    delta_time: " << mtrk_event.delta_time << '\n';
      switch (mtrk_event.event.type) {
      case midi::event_t::type_t::midi:
	std::cout << "    midi: " << '\n';
	break;
      case midi::event_t::type_t::sysex:
	std::cout << "    sysex: " << '\n';
	break;
      case midi::event_t::type_t::meta:
	std::cout << "    meta: " << '\n';
	break;
      default:
	assert(false);
      }
    }

    assert(buf - track_start == track_length);
  }
  std::cout << "trailing/unparsed data: " << size - (buf - start) << '\n';
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    std::cerr << "argc < 2\n";
    return -1;
  }

  std::cerr << argv[1] << '\n';

  std::ifstream ifs;
  ifs.open(argv[1], std::ios::binary | std::ios::ate);
  if (!ifs.is_open()) {
    std::cerr << "ifstream\n";
    return -1;
  }

  auto size = static_cast<int32_t>(ifs.tellg());
  uint8_t start[size];
  ifs.seekg(0);
  if (!ifs.read(reinterpret_cast<char*>(&start[0]), size)) {
    std::cerr << "read\n";
    return -1;
  }

  parse(start);

  return 0;
}
