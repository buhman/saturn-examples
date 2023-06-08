#pragma once

#include <stdint.h>

#include "../common/copy.hpp"
#include "../common/minmax.hpp"

namespace editor {

template <int C>
struct line {
  uint8_t buf[C];
  int32_t length;
};

struct cursor {
  int32_t row;
  int32_t col;
};

template <int C, int R>
struct buffer {
  line<C> row[R];
  line<C> * lines[R];
  int32_t length;
  int32_t alloc_ix;
  struct {
    int32_t cell_width;
    int32_t cell_height;
    int32_t top;
    int32_t left;
  } window;
  struct cursor cursor;

  typedef line<C> line_type;

  inline static constexpr int32_t rows_max_length(){return R;}
  inline static constexpr int32_t cols_max_length(){return C;}

  inline constexpr buffer(int32_t width, int32_t height);
  inline constexpr line<C> * allocate();
  inline constexpr void deallocate(line<C> *& l);
  inline constexpr bool put(uint8_t c);
  inline constexpr bool backspace();
  inline constexpr bool cursor_left();
  inline constexpr bool cursor_right();
  inline constexpr bool cursor_up();
  inline constexpr bool cursor_down();
  inline constexpr bool cursor_home();
  inline constexpr bool cursor_end();
  inline constexpr bool enter();
private:
  inline constexpr void scroll_left();
  inline constexpr void scroll_right();
  inline constexpr void scroll_up();
  inline constexpr void scroll_down();
};

template <int C, int R>
inline constexpr buffer<C, R>::buffer(int32_t width, int32_t height)
{
  this->length = 1;
  this->alloc_ix = 0;
  this->window.top = 0;
  this->window.left = 0;
  this->window.cell_height = height;
  this->window.cell_width = width;
  this->cursor.row = 0;
  this->cursor.col = 0;

  for (int32_t i = 0; i < R; i++) {
    this->row[i].length = -1;
    this->lines[i] = nullptr;

    for (int32_t j = 0; j < C; j++) {
      this->row[i].buf[j] = 0x7f;
    }
  }
}

template <int C, int R>
inline constexpr line<C> * buffer<C, R>::allocate()
{
  line<C> * l;
  while ((l = &this->row[this->alloc_ix])->length != -1) {
    // R must be a power of 2
    static_assert((R & (R - 1)) == 0);
    this->alloc_ix = (this->alloc_ix + 1) & (R - 1);
  }
  l->length = 0;
  return l;
}

template <int C, int R>
inline constexpr void buffer<C, R>::deallocate(line<C> *& l)
{
  // does not touch alloc_ix
  fill<uint8_t>(l->buf, 0x7f, l->length);
  l->length = -1;
  l = nullptr;
}

template <int C, int R>
inline constexpr bool buffer<C, R>::put(const uint8_t c)
{
  struct cursor& cur = this->cursor;
  line<C> *& l = this->lines[cur.row];
  if (l == nullptr) {
    l = this->allocate();
  }

  //   v
  // 0123
  if (l->length >= C || cur.col >= C || cur.col < 0)
    return false;

  if (l->length > cur.col) {
    //    c (5)
    // 01234
    // 012a34
    // 4, 3, (5 - 3)
    move(&l->buf[cur.col+1], &l->buf[cur.col], l->length - cur.col);
  }
  l->buf[cur.col] = c;
  l->length++;
  cur.col++;
  scroll_right();

  return true;
}

template <int C, int R>
inline constexpr bool buffer<C, R>::backspace()
{
  struct cursor& cur = this->cursor;

  if (cur.col < 0 || cur.col > C)
    return false;

  line<C> *& l = this->lines[cur.row];
  if (l == nullptr) {
    // is it possible to get in this state?
    return false;
  }
  if (l->length < 0) {
    // is it possible to get in this state?
    return false;
  }

  if (cur.col == 0) {
    if (cur.row == 0) return false;
    // combine this line with the previous line
    line<C> *& lp = this->lines[cur.row-1];
    if (lp == nullptr) lp = allocate();
    // blindly truncate overflowing lines
    auto length = min(l->length - cur.col, C - lp->length);
    if (length > 0) move(&lp->buf[lp->length], &l->buf[cur.col], length);

    cur.col = lp->length;
    scroll_right();
    lp->length += length;
    deallocate(l);
    // 0 a
    // 1
    // 2 _ (cur.row, deleted)
    // 3 b
    int32_t n_lines = this->length - (cur.row + 1);
    move(&this->lines[cur.row], &this->lines[cur.row+1], (sizeof (line<C>*)) * n_lines);
    this->length--;
    this->lines[this->length] = nullptr;
    cur.row--;
    scroll_up();
  } else {
    //    c
    // 01234
    auto length = l->length - cur.col;
    if (length > 0) move(&l->buf[cur.col-1], &l->buf[cur.col], length);

    cur.col--;
    scroll_left();
    l->length--;
    l->buf[l->length] = 0x7f;
  }

  return true;
}

template <int C, int R>
inline constexpr bool buffer<C, R>::cursor_left()
{
  struct cursor& cur = this->cursor;

  if (cur.col <= 0) {
    if (cur.row <= 0)
      return false;

    cur.row--;
    scroll_up();

    line<C> const * const l = this->lines[cur.row];
    int32_t length = (l == nullptr) ? 0 : l->length;
    cur.col = length;
    scroll_right();
  } else {
    cur.col--;
    scroll_left();
  }

  return true;
}

template <int C, int R>
inline constexpr bool buffer<C, R>::cursor_right()
{
  struct cursor& cur = this->cursor;

  line<C> const * const l = this->lines[cur.row];
  int32_t length = (l == nullptr) ? 0 : l->length;
  if (cur.col >= length) {
    if (cur.row >= this->length)
      return false;

    cur.row++;
    scroll_down();

    cur.col = 0;
    scroll_left();
  } else {
    cur.col++;
    scroll_right();
  }

  return true;
}

template <int C, int R>
inline constexpr bool buffer<C, R>::cursor_up()
{
  struct cursor& cur = this->cursor;

  if (cur.row <= 0)
    return false;

  cur.row--;
  scroll_up();

  line<C> const * const l = this->lines[cur.row];
  int32_t length = (l == nullptr) ? 0 : l->length;
  cur.col = min(cur.col, length);
  scroll_left();
  return true;
}

template <int C, int R>
inline constexpr bool buffer<C, R>::cursor_down()
{
  struct cursor& cur = this->cursor;

  if (cur.row >= this->length)
    return false;

  cur.row++;
  scroll_down();

  line<C> const * const l = this->lines[cur.row];
  int32_t length = (l == nullptr) ? 0 : l->length;
  cur.col = min(cur.col, length);
  scroll_left();
  return true;
}

template <int C, int R>
inline constexpr bool buffer<C, R>::cursor_home()
{
  struct cursor& cur = this->cursor;

  line<C> const * const l = this->lines[cur.row];
  if (l == nullptr)
    return false;

  cur.col = 0;
  scroll_left();
  return true;
}

template <int C, int R>
inline constexpr bool buffer<C, R>::cursor_end()
{
  struct cursor& cur = this->cursor;

  line<C> const * const l = this->lines[cur.row];
  if (l == nullptr)
    return false;

  cur.col = l->length;
  scroll_right();
  return true;
}

template <int C, int R>
inline constexpr bool buffer<C, R>::enter()
{
  struct cursor& cur = this->cursor;

  if (cur.row >= R || cur.row < 0)
    return false;

  if (this->length > cur.row) {
    // first, move all lines down one
    int32_t n_lines = this->length - (cur.row + 1);
    // don't care about nullptr here, this never dereferences
    move(&this->lines[cur.row+2], &this->lines[cur.row+1], (sizeof (line<C>*)) * n_lines);
  }
  // column-wise copy of the cursor position to the newly-created line
  line<C> * old_l = this->lines[cur.row];
  line<C> * new_l = allocate();
  //   v
  // 01234 (5)
  if (old_l != nullptr) {
    new_l->length = old_l->length - cur.col;
    if (new_l->length > 0) {
      old_l->length -= new_l->length;
      copy(&new_l->buf[0], &old_l->buf[cur.col], new_l->length);
      fill<uint8_t>(&old_l->buf[cur.col], 0x7f, new_l->length);
    }
  } else {
    // nothing to do, new_l->length is already 0
  }

  cur.row++;
  scroll_down();
  this->lines[cur.row] = new_l;
  cur.col = 0;
  scroll_left();
  // because copy() is used instead of put(), length needs to be
  // incremented here.
  this->length++;
  return true;
}

template <int C, int R>
inline constexpr void buffer<C, R>::scroll_up()
{
  if (this->cursor.row < this->window.top)
    this->window.top = this->cursor.row;
}

template <int C, int R>
inline constexpr void buffer<C, R>::scroll_down()
{
  // 0: a  -
  // 1: bv -

  // 0: a
  // 1: b  -
  // 2: cv -

  if (this->cursor.row >= this->window.top + this->window.cell_height)
    this->window.top = (this->cursor.row - (this->window.cell_height - 1));
}

template <int C, int R>
inline constexpr void buffer<C, R>::scroll_left()
{
  if (this->cursor.col < this->window.left)
    this->window.left = this->cursor.col;
}

template <int C, int R>
inline constexpr void buffer<C, R>::scroll_right()
{
  // 0: a  -
  // 1: bv -

  // 0: a
  // 1: b  -
  // 2: cv -

  if (this->cursor.col >= this->window.left + this->window.cell_width)
    this->window.left = (this->cursor.col - (this->window.cell_width - 1));
}

}
