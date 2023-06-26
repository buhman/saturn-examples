#pragma once

#include <string>

#include "midi.hpp"

namespace midi {
namespace strings {

constexpr inline std::string
header_format(header_t::format_t format)
{
  switch (format) {
  case header_t::format_t::_0: return "0";
  case header_t::format_t::_1: return "1";
  case header_t::format_t::_2: return "2";
  }
  while (1);
}

}
}
