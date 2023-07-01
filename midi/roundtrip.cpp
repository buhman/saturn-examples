#include <iostream>
#include <fstream>
#include <cassert>
#include <cstring>
#include <optional>

#include "strings.hpp"
#include "parse.hpp"
#include "generate.hpp"

#include "iterator.cpp"

static char output_buf[4096];

inline uint32_t output_mtrk_event(const midi::mtrk_event_t& mtrk_event, uint8_t& running_status,
				  std::ofstream& ofs, const bool write)
{
  uint8_t * output_buf_start = reinterpret_cast<uint8_t *>(&output_buf[0]);
  const uint8_t * output_buf_end = midi::generate::mtrk_event(output_buf_start, mtrk_event, running_status);
  const uint32_t event_length = output_buf_end - output_buf_start;

  std::cerr << "event_length " << event_length << '\n';

  if (write) ofs.write(output_buf, event_length);

  return event_length;
}

inline void output_track(const midi::track_t& track, std::ofstream& ofs)
{
  uint8_t * output_buf_start = reinterpret_cast<uint8_t *>(&output_buf[0]);
  const uint8_t * output_buf_end = midi::generate::track(output_buf_start, track);
  const uint32_t track_length = output_buf_end - output_buf_start;

  ofs.write(output_buf, track_length);
}

inline void output_header(const midi::header_t& header, std::ofstream& ofs)
{
  uint8_t * output_buf_start = reinterpret_cast<uint8_t *>(&output_buf[0]);
  const uint8_t * output_buf_end = midi::generate::header(output_buf_start, header);
  const uint32_t header_length = output_buf_end - output_buf_start;

  ofs.write(output_buf, header_length);
}

constexpr inline bool
init_track_iterators(const midi::header_t& header, mtrk_iterator * its, uint8_t const * buf)
{
  for (int32_t i = 0; i < header.ntrks; i++) {
    auto track_o = midi::parse::track(buf);
    if (!track_o) {
      std::cerr << "invalid track\n";
      return false;
    }

    midi::track_t track;
    std::tie(buf, track) = *track_o;

    std::cout << "track[" << i << "] track.length: " << track.length << '\n';

    uint8_t const * const track_start = buf;
    its[i] = mtrk_iterator(track, track_start);

    buf += track.length;
  }
  return true;
}

inline std::optional<uint32_t>
simulate_delta_time(const midi::header_t& header, uint8_t const * buf,
		    std::ofstream& ofs, const bool write)
{
  //
  // simulate delta_time
  //
  using mtrk_storage = std::aligned_storage_t<sizeof(mtrk_iterator), alignof(mtrk_iterator)>;
  mtrk_storage its_storage[header.ntrks];
  auto &its = reinterpret_cast<mtrk_iterator (&)[header.ntrks]>(its_storage);

  if (!init_track_iterators(header, its, buf))
    return std::nullopt;
  uint32_t track_times[header.ntrks] = {0};

  uint32_t global_time = 0;
  uint32_t last_global_time = 0;
  int32_t complete = 0;
  uint32_t total_length = 0;
  uint8_t output_running_status = 0;
  do {
    complete = 0;
    for (uint32_t i = 0; i < header.ntrks; i++) {
      mtrk_iterator& it = its[i];
      uint32_t& track_time = track_times[i];

      while (!it.at_end()) {
	const midi::mtrk_event_t& mtrk_event = *it;
	if (track_time + mtrk_event.delta_time <= global_time) {
	  track_time += mtrk_event.delta_time;
	  ++it;
	} else {
	  break;
	}

	if (mtrk_event.event.type == midi::event_t::type_t::meta
	    && mtrk_event.event.event.meta.type == 0x2f) {
	  // suppress end of track
	  std::cout << "suppress EOT\n";
	  continue;
	}
	
	total_length += output_mtrk_event({
	  global_time - last_global_time,
	  mtrk_event.event
	}, output_running_status, ofs, write);
	last_global_time = global_time;
      }
      if (it.at_end()) ++complete;
    }
    // increment global time to the next time step
    ++global_time;
  } while (complete != header.ntrks);


  midi::mtrk_event_t mtrk_event_eot = {
    0,
    { // event_t
      .type = midi::event_t::type_t::meta,
      .event = {
	.meta = { nullptr, 0, 0x2f } // EndOfTrack
      }
    }
  };
  // emit final EndOfTrack
  total_length += output_mtrk_event(mtrk_event_eot, output_running_status, ofs, write);
  last_global_time = global_time;

  return total_length;
}

int roundtrip(uint8_t const * start, std::ofstream& ofs)
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

  //
  // write output header
  //
  output_header({
    .format = midi::header_t::format_t::_0,
    .ntrks = 1,
    .division = header.division,
  }, ofs);

  //
  // first round of simulation: calculate track length (in bytes)
  //
  auto track_length_o = simulate_delta_time(header, buf, ofs, false);
  if (!track_length_o) {
    return -1;
  }

  output_track({ *track_length_o }, ofs);

  //
  // second round of simulation: write the actual track
  //
  auto _o = simulate_delta_time(header, buf, ofs, true);
  if (!_o) {
    return -1;
  }

  return 0;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    std::cerr << "argc < 3\n";
    return -1;
  }

  std::cerr << argv[1] << '\n';

  std::ifstream ifs;
  ifs.open(argv[1], std::ios::in | std::ios::binary | std::ios::ate);
  if (!ifs.is_open()) {
    std::cerr << "ifstream: " << '\n';
    return -1;
  }

  auto size = static_cast<int32_t>(ifs.tellg());
  uint8_t start[size];
  ifs.seekg(0);
  if (!ifs.read(reinterpret_cast<char*>(&start[0]), size)) {
    std::cerr << "read\n";
    return -1;
  }
  ifs.close();

  std::ofstream ofs;
  ofs.open(argv[2], std::ios::out | std::ios::binary | std::ios::trunc);
  if (!ofs.is_open()) {
    std::cerr << "ofstream\n";
    return -1;
  }

  roundtrip(start, ofs);

  ofs.close();

  return 0;
}
