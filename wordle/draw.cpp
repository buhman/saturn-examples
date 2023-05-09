#include "wordle.hpp"

namespace wordle {
  namespace draw {

void keyboard()
{
}

static inline void guess(struct row const& r, void (*draw_char)(uint8_t, int32_t, int32_t))
{
  for (uint32_t i = 0; i < word_length; i++) {
    uint8_t l = r.letters[i];

    draw_char(l, 0, 0);

    break;
  }
}

void guesses(struct screen& s, void (*draw_char)(uint8_t, int32_t, int32_t))
{
  guess(s.rows[0], draw_char);
}

// end namespace
  }
}
