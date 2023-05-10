#include <stdint.h>
#include <stdbool.h>

#include "wordle.hpp"

namespace wordle {

void init_screen(struct screen& s, const uint8_t * word)
{
  s.edit.row = 0;
  s.edit.index = 0;

  for (uint32_t j = 0; j < word_length; j++) {
    for (uint32_t i = 0; i < guesses; i++) {
      s.rows[i].letters[j] = ' ';
    }

    s.word[j] = word[j];
  }

  s.rows[0].letters[0] = 'A';
  s.rows[0].letters[1] = 'B';
  s.rows[0].letters[2] = 'C';
  s.rows[0].letters[3] = 'D';
  s.rows[0].letters[4] = 'E';

  s.rows[1].letters[0] = 'F';
  s.rows[1].letters[1] = 'G';
  s.rows[1].letters[2] = 'H';
  s.rows[1].letters[3] = 'I';
  s.rows[1].letters[4] = 'J';

  s.rows[2].letters[0] = 'K';
  s.rows[2].letters[1] = 'L';
  s.rows[2].letters[2] = 'M';
  s.rows[2].letters[3] = 'N';
  s.rows[2].letters[4] = 'O';

  s.rows[3].letters[0] = 'P';
  s.rows[3].letters[1] = 'Q';
  s.rows[3].letters[2] = 'R';
  s.rows[3].letters[3] = 'S';
  s.rows[3].letters[4] = 'T';

  s.rows[4].letters[0] = 'U';
  s.rows[4].letters[1] = 'V';
  s.rows[4].letters[2] = 'W';
  s.rows[4].letters[3] = 'X';
  s.rows[4].letters[4] = 'Y';

  s.rows[5].letters[0] = 'Z';
  s.rows[5].letters[1] = '1';
  s.rows[5].letters[2] = '2';
  s.rows[5].letters[3] = '3';
  s.rows[5].letters[4] = '4';

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
