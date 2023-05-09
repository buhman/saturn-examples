#include <stdint.h>
#include <stdbool.h>

#include "wordle.hpp"

namespace wordle {

void init_game(struct screen& s, const uint8_t * word)
{
  s.edit.row = 0;
  s.edit.index = 0;

  for (uint32_t j = 0; j < word_length; j++) {
    for (uint32_t i = 0; i < guesses; i++) {
      s.rows[i].letters[j] = ' ';
    }

    s.word[j] = word[j];
  }

  s.all_letters = 0;
}

bool type_letter(struct screen& s, const uint8_t letter)
{
  if (s.edit.index >= word_length)
    return false;
  if (!(letter >= 'a' && letter <= 'z'))
    return false;

  struct row& r = s.rows[s.edit.row];

  r.letters[s.edit.index++] = letter;

  return true;
}

bool backspace(struct screen& s)
{
  if (s.edit.index <= 0)
    return false;

  struct row& r = s.rows[s.edit.row];

  r.letters[--s.edit.index] = ' ';

  return true;
}

}
