#include <stdint.h>

struct track {
  const uint8_t control_adr;
  const uint8_t fad[3];
};
static_assert((sizeof (track)) == 4);

struct start_track {
  const uint8_t control_adr;
  const uint8_t start_track_number;
  const uint8_t _zero[2];
};
static_assert((sizeof (start_track)) == 4);

struct end_track {
  const uint8_t control_adr;
  const uint8_t end_track_number;
  const uint8_t _zero[2];
};
static_assert((sizeof (end_track)) == 4);

struct toc {
  struct track track[99];
  struct start_track start_track;
  struct end_track end_track;
  struct track lead_out;
};
static_assert((sizeof (toc)) == 408);

uint32_t track_fad(struct track * t) const
{
  return (t->fad[0] << 16) | (t->fad[1] << 8) | (t->fad[2] << 0);
}

uint8_t track_control(struct track * t) const
{
  return (t->control_adr >> 4) & 0xf;
}

uint8_t track_adr(struct track * t) const
{
  return (t->control_adr >> 0) & 0xf;
}
