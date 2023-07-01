#pragma once

#include <optional>
#include <tuple>
#include <cstdint>

#include "midi.hpp"

namespace midi {
namespace parse {

using buf_t = uint8_t const *;

std::optional<std::tuple<buf_t, header_t>>
header(buf_t buf);

std::optional<std::tuple<buf_t, track_t>>
track(buf_t buf);

std::optional<std::tuple<buf_t, mtrk_event_t>>
mtrk_event(buf_t buf, uint8_t& running_status);

}
}
