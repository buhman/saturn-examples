#pragma once

#include <cstdint>

namespace midi {

struct division_t {
  enum struct type_t : uint8_t {
    time_code,
    metrical,
  } type;
  union {
    struct {
      int8_t smpte;
      uint8_t ticks_per_frame;
    } time_code;
    struct {
      uint16_t ticks_per_quarter_note;
    } metrical;
  };
};

struct header_t {
  enum struct format_t : uint8_t {
    _0,
    _1,
    _2,
  } format;
  uint16_t ntrks;
  division_t division;
};

struct midi_event_t {
  struct note_off_t {
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
  };

  struct note_on_t {
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
  };

  struct polyphonic_key_pressure_t {
    uint8_t channel;
    uint8_t note;
    uint8_t pressure;
  };

  struct control_change_t {
    uint8_t channel;
    uint8_t control;
    uint8_t value;
  };

  struct program_change_t {
    uint8_t channel;
    uint8_t program;
  };

  struct channel_pressure_t {
    uint8_t channel;
    uint8_t pressure;
  };

  struct pitch_bend_change_t {
    uint8_t channel;
    uint8_t lsb;
    uint8_t msb;
  };

  struct channel_mode_t {
    uint8_t channel;
    uint8_t controller;
    uint8_t value;
  };

  enum struct type_t {
    note_off,
    note_on,
    polyphonic_key_pressure,
    control_change,
    program_change,
    channel_pressure,
    pitch_bend_change,
    channel_mode,
  } type;
  union event_t {
    note_off_t note_off;
    note_on_t note_on;
    polyphonic_key_pressure_t polyphonic_key_pressure;
    control_change_t control_change;
    program_change_t program_change;
    channel_pressure_t channel_pressure;
    pitch_bend_change_t pitch_bend_change;
    channel_mode_t channel_mode;
  } data;
};

struct sysex_event_t {
  const uint8_t * data;
  uint32_t length;
};

struct meta_event_t {
  const uint8_t * data;
  uint32_t length;
  uint8_t type;
};

struct event_t {
  enum struct type_t {
    midi,
    sysex,
    meta,
  } type;
  union _event_t {
    midi_event_t midi;
    sysex_event_t sysex;
    meta_event_t meta;
  } event;
};

struct mtrk_event_t {
  uint32_t delta_time;
  event_t event;
};

} // midi
