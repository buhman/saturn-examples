#include <stdint.h>

namespace wordle {

constexpr uint32_t word_length = 5;
constexpr uint32_t guesses = 6;

enum class clue {
  exact,
  present,
  absent,
  none
};

struct row {
  uint8_t letters[word_length];
  enum clue clues[word_length];
};

struct edit {
  uint32_t row;
  uint32_t index;
};

struct screen {
  struct edit edit;
  struct row rows[guesses];
  uint32_t word_ix;
  enum clue clues[26];
};

void init_screen(struct screen& s, uint32_t word_ix);
bool type_letter(struct screen& s, const uint8_t letter);
bool backspace(struct screen& s);
bool confirm_word(struct screen& s);
}
