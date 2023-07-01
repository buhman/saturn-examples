#include <iterator>
#include <tuple>
#include <cassert>
#include <type_traits>

#include "midi.hpp"
#include "parse.hpp"

struct mtrk_iterator {
  using difference_type = int32_t;
  using element_type = midi::mtrk_event_t;
  using pointer = element_type *;
  using reference = element_type &;

  uint8_t const * track_start;
  midi::track_t track;
  uint8_t running_status;
  uint8_t const * buf;
  uint8_t const * next_buf;
  midi::mtrk_event_t mtrk_event;

  mtrk_iterator() = delete;
  mtrk_iterator(const midi::track_t& track,
		uint8_t const * const track_start)
    : track_start(track_start)
    , track(track)
    , running_status(0)
    , buf(track_start)
    , next_buf(track_start)
  {
  }

  mtrk_iterator& operator=(mtrk_iterator&&) = default;
  constexpr mtrk_iterator(const mtrk_iterator&) = default;

  reference operator*() {
    auto mtrk_event_o = midi::parse::mtrk_event(buf, running_status);
    std::tie(next_buf, mtrk_event) = *mtrk_event_o;
    return mtrk_event;
  }

  mtrk_iterator operator++() {
    assert(buf != next_buf);
    buf = next_buf;
    return *this;
  }

  mtrk_iterator operator++(int) {
    mtrk_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

  bool at_end() {
    return buf - track_start >= track.length;
  }

};
