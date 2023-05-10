#include <stdint.h>
#include <stdbool.h>

#include "wordle.hpp"
#include "word_list.hpp"

namespace wordle {

void init_screen(struct screen& s, uint32_t rand)
{
  s.edit.row = 0;
  s.edit.index = 0;

  for (uint32_t j = 0; j < word_length; j++) {
    for (uint32_t i = 0; i < guesses; i++) {
      s.rows[i].letters[j] = ' ';
      s.rows[i].clues[j] = clue::none;
    }

  }

  for (uint32_t k = 0; k < 26; k++) {
    s.clues[k] = clue::none;
  }

  constexpr uint32_t answer_length = (sizeof (answers)) / (sizeof (uint32_t));
  s.word_ix = answers[rand % answer_length];
}

bool type_letter(struct screen& s, const uint8_t letter)
{
  if (s.edit.row >= guesses)
    return false;

  uint8_t l;

  if (s.edit.index >= word_length)
    return false;
  if (letter >= 'a' && letter <= 'z') {
    l = letter - ('a' - 'A'); // upcase
  } else if (letter >= 'A' && letter <= 'Z') {
    l = letter;
  } else {
    return false;
  }

  struct row& r = s.rows[s.edit.row];

  r.letters[s.edit.index++] = l;

  return true;
}

bool backspace(struct screen& s)
{
  if (s.edit.row >= guesses)
    return false;

  if (s.edit.index <= 0)
    return false;

  struct row& r = s.rows[s.edit.row];

  r.letters[--s.edit.index] = ' ';

  return true;
}

static inline int32_t word_in_list(const uint8_t * word)
{
  constexpr uint32_t word_list_items = (sizeof word_list) / (word_length);

  for (uint32_t i = 0; i < word_list_items; i++) {
    for (uint32_t j = 0; j < word_length; j++) {
      if (word_list[i][j] != word[j])
        goto next_word;
    }
    // word_length loop exited and all letters matched
    return true;

  next_word:
    continue;
  }
  // exhausted word list; nothing matches
  return false;
}

constexpr inline enum clue next_clue(enum clue a, enum clue b)
{
  if (a == clue::exact || b == clue::exact)
    return clue::exact;
  if (a == clue::present || b == clue::present)
    return clue::present;
  if (a == clue::absent || b == clue::absent)
    return clue::absent;

  return clue::none;
}

static inline void update_clues(enum clue * clues, struct row &r, const uint8_t * word)
{
  for (uint32_t i = 0; i < word_length; i++) {
    enum clue &c = r.clues[i];

    c = clue::absent;
    const uint8_t l = r.letters[i];

    if (l == word[i]) {
      c = clue::exact;
    } else {
      // also check for position errors
      for (uint32_t j = 0; j < word_length; j++) {
        if (word[j] == l) {
          c = clue::present;
          break;
        }
      }
    }

    int32_t l_ix = static_cast<int32_t>(l) - static_cast<int32_t>('A');
    if (l_ix >= 0 && l_ix < 26) {
      // update global clues
      clues[l_ix] = next_clue(clues[l_ix], c);
    }
  }
}

bool confirm_word(struct screen& s)
{
  if (s.edit.row >= guesses)
    return false;

  if (s.edit.index < word_length)
    return false;

  if (!(word_in_list(s.rows[s.edit.row].letters)))
    return false;

  update_clues(s.clues, s.rows[s.edit.row], word_list[s.word_ix]);

  s.edit.row++;
  s.edit.index = 0;

  return true;
}


}
