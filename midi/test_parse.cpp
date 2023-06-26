#include "parse.cpp"

using buf_a_t = uint8_t const [];

using namespace midi;
using namespace midi::parse;

constexpr buf_a_t test_ifl16 = {0x12, 0x34};
static_assert(int_fixed_length16(test_ifl16)
	      ==
	      std::tuple<buf_t, uint16_t>({&test_ifl16[2], 0x1234}));

constexpr buf_a_t test_ifl32 = {0x12, 0x34, 0x56, 0x78};
static_assert(int_fixed_length32(test_ifl32)
	      ==
	      std::tuple<buf_t, uint32_t>({&test_ifl32[4], 0x12345678}));

constexpr buf_a_t test_header = {0x4d, 0x54, 0x68, 0x64, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x60};
static_assert(header(test_header) != std::nullopt);
constexpr header_t h1 = std::get<1>(*header(test_header));
static_assert(h1.format == header_t::format_t::_0);
static_assert(h1.ntrks == 1);
static_assert(h1.division.type == division_t::type_t::metrical);
static_assert(h1.division.metrical.ticks_per_quarter_note == 96);

constexpr buf_a_t test_met1 = {0x8a};
constexpr buf_a_t test_met2 = {0xb2, 121};
constexpr buf_a_t test_met3 = {0xb3, 0};
constexpr midi_event_t::type_t met1 = std::get<1>(*midi_event_type(test_met1));
constexpr midi_event_t::type_t met2 = std::get<1>(*midi_event_type(test_met2));
constexpr midi_event_t::type_t met3 = std::get<1>(*midi_event_type(test_met3));
static_assert(met1 == midi_event_t::type_t::note_off);
static_assert(met2 == midi_event_t::type_t::channel_mode);
static_assert(met3 == midi_event_t::type_t::control_change);
