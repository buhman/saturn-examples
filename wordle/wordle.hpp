#include <stdint.h>

namespace wordle {

constexpr uint32_t word_length = 5;
constexpr uint32_t guesses = 6;

enum class clue {
  correct,
  position,
  absent,
  none
};

struct row {
  uint8_t letters[word_length];
};

struct edit {
  uint32_t row;
  uint32_t index;
};

struct screen {
  struct edit edit;
  struct row rows[guesses];
  uint8_t word[word_length];
  uint32_t all_letters;
};

void init_screen(struct screen& s, const uint8_t * word);

}
