#pragma once

#include <cstdint>

#include "midi.hpp"

namespace midi {
namespace generate {

using buf_t = uint8_t *;

buf_t
header(buf_t buf, const header_t& header) noexcept;

buf_t
track(buf_t buf, const track_t& track) noexcept;

buf_t
mtrk_event(buf_t buf, const mtrk_event_t& event, uint8_t& running_status) noexcept;

}
}
