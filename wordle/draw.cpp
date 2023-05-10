#include "wordle.hpp"

namespace wordle {
  namespace draw {

constexpr int32_t box_dim = 21;
constexpr int32_t grid_space = box_dim + 2;

const static uint8_t layout[3][10] = {
  {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
    {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L'},
      {'Z', 'X', 'C', 'V', 'B', 'N', 'M'},
};


void keyboard(struct screen const& s, void (*draw_char)(uint8_t, int32_t, int32_t, int32_t, int32_t))
{
  constexpr int32_t origin_x[3] = {46, 57, 69};
  constexpr int32_t origin_y = 170;
  constexpr uint32_t rows = 3;
  constexpr uint32_t cols[3] = {10, 9, 7};

  for (uint32_t row = 0; row < rows; row++) {
    for (uint32_t col = 0; col < cols[row]; col++) {
      uint8_t l = layout[row][col];

      int32_t x1 = origin_x[row] + (col * grid_space);
      int32_t y1 = origin_y + (row * grid_space);
      int32_t x2 = x1 + box_dim;
      int32_t y2 = y1 + box_dim;

      draw_char(l, x1, y1, x2, y2);
    }
  }
}

void guesses(struct screen const& s, void (*draw_char)(uint8_t, int32_t, int32_t, int32_t, int32_t))
{
  // first row is at (104,23).
  // midpoint is +(10,10)
  // grid is +(13,13)

  constexpr int32_t origin_x = 103;
  constexpr int32_t origin_y = 17;

  for (uint32_t row = 0; row < wordle::guesses; row++) {

    struct row const& r = s.rows[row];

    for (uint32_t col = 0; col < word_length; col++) {
      uint8_t l = r.letters[col];

      int32_t x1 = origin_x + (col * grid_space);
      int32_t y1 = origin_y + (row * grid_space);
      int32_t x2 = x1 + box_dim;
      int32_t y2 = y1 + box_dim;

      draw_char(l, x1, y1, x2, y2);
    }
  }
}

// end namespace
  }
}
