#pragma once

#include <stdint.h>

#include "../common/copy.hpp"
#include "../common/minmax.hpp"

namespace editor {

template <int C>
struct line {
  uint8_t buf[C];
  int32_t length;
  int32_t refcount;

  static_assert(C < 32768);
};

struct cursor {
  int32_t col;
  int32_t row;
};

enum struct mode {
  normal,
  mark,
};

struct selection {
  cursor * min;
  cursor * max;

  inline constexpr bool contains(int32_t col, int32_t row);
};

inline constexpr bool selection::contains(int32_t col, int32_t row)
{
  if (this->min != this->max) {
    if (row == this->min->row && row == this->max->row) {
      if (col >= this->min->col && col < this->max->col)
	return true;
    } else {
      if (  (row == this->min->row && col >= this->min->col)
	 || (row == this->max->row && col  < this->max->col)
	 || (row  > this->min->row && row  < this->max->row))
	return true;
    }
  }
  return false;
}

template <int C, int R, int V>
struct buffer {
  static_assert((C & (C - 1)) == 0);
  static_assert(C < 32 || C % 32 == 0);

  line<C> row[R];
  line<C> * lines[R];
  int32_t length;
  struct {
    line<C> * lines[R];
    int32_t length;
  } shadow;
  int32_t alloc_ix;
  struct {
    int32_t cell_width;
    int32_t cell_height;
    int32_t top;
    int32_t left;
    int32_t viewport[V][C < 32 ? 1 : C / 32];
  } window;
  struct cursor cursor;
  struct cursor mark;
  enum mode mode;

  typedef line<C> line_type;

  inline static constexpr int32_t rows_max_length(){return R;}
  inline static constexpr int32_t cols_max_length(){return C;}

  inline constexpr buffer(int32_t width, int32_t height);
  inline constexpr line<C> * allocate();
  inline static constexpr void deallocate(line<C> *& l);
  inline static constexpr void decref(line<C> *& l);
  inline constexpr line<C> * dup(line<C> const * const l);
  inline constexpr line<C> * mutref(line<C> *& l);
  inline static constexpr line<C> * incref(line<C> * l);
  inline static constexpr int32_t line_length(line<C> const * const l);
  inline constexpr bool put(uint8_t c);
  inline constexpr bool delete_forward();
  inline constexpr bool delete_backward();
  inline constexpr void delete_word_forward();
  inline constexpr void delete_word_backward();

  inline constexpr bool cursor_left();
  inline constexpr bool cursor_right();
  inline constexpr bool cursor_up();
  inline constexpr bool cursor_down();
  inline constexpr bool cursor_home();
  inline constexpr bool cursor_end();
  inline constexpr uint8_t cursor_get(const editor::cursor& cur);
  inline constexpr bool cursor_increment(editor::cursor& cur);
  inline constexpr void cursor_scan_word_forward();
  inline constexpr bool cursor_decrement(editor::cursor& cur);
  inline constexpr void cursor_scan_word_backward();

  inline constexpr bool enter();
  inline constexpr void line_kill();
  inline constexpr void mark_set();
  inline constexpr selection mark_get();
  inline constexpr void delete_from_line(line<C> *& l,
					 const int32_t col_start_ix,
					 const int32_t col_end_ix);
  inline constexpr void selection_delete(const selection& sel);
  inline constexpr bool mark_delete();
  inline constexpr void quit();
  inline constexpr void shadow_clear();
  inline constexpr void _shadow_cow(line<C> * src,
				    const int32_t dst_row_ix,
				    const int32_t col_start_ix,
				    const int32_t col_end_ix);
  inline constexpr bool shadow_copy();
  inline constexpr bool shadow_cut();
  inline constexpr void overwrite_line(line<C> *& dst,
				       const int32_t dst_col,
				       line<C> * src,
				       const int32_t src_col);
  inline constexpr bool shadow_paste();
  inline constexpr void scroll_left();
  inline constexpr void scroll_right();
  inline constexpr void scroll_up();
  inline constexpr void scroll_down();
  inline constexpr void scroll_new_cursor(const editor::cursor& cur);
  inline constexpr void scroll_new_col(const int32_t col);
};

template <int C, int R, int V>
inline constexpr buffer<C, R, V>::buffer(int32_t width, int32_t height)
{
  this->length = 1;
  this->shadow.length = 1;
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
    this->shadow.lines[i] = nullptr;

    /*
    for (int32_t j = 0; j < C; j++) {
      this->row[i].buf[j] = 0x7f;
    }
    */
  }
}

template <int C, int R, int V>
inline constexpr line<C> * buffer<C, R, V>::allocate()
{
  line<C> * l;
  while ((l = &this->row[this->alloc_ix])->length != -1) {
    // R must be a power of 2
    static_assert((R & (R - 1)) == 0);
    this->alloc_ix = (this->alloc_ix + 1) & (R - 1);
  }
  l->length = 0;
  l->refcount = 1;
  return l;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::deallocate(line<C> *& l)
{
  // does not touch alloc_ix
  l->length = -1;
  l->refcount = 0;
  l = nullptr;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::decref(line<C> *& l)
{
  if (l == nullptr) return;
  else if (l->refcount == 1)
    buffer<C, R, V>::deallocate(l);
  else {
    l->refcount--;
    l = nullptr;
  }
}

template <int C, int R, int V>
inline constexpr line<C> * buffer<C, R, V>::dup(line<C> const * const src)
{
  line<C> * dst = this->allocate();
  copy(&dst->buf[0], &src->buf[0], src->length);
  dst->length = src->length;
  return dst;
}

template <int C, int R, int V>
inline constexpr line<C> * buffer<C, R, V>::mutref(line<C> *& l)
{
  if (l == nullptr) {
    l = this->allocate();
  } else if (l->refcount > 1) {
    // copy-on-write
    l->refcount--;
    l = this->dup(l);
  }
  return l;
}

template <int C, int R, int V>
inline constexpr line<C> * buffer<C, R, V>::incref(line<C> * l)
{
  if (l != nullptr) l->refcount++;
  return l;
}

template <int C, int R, int V>
inline constexpr int32_t buffer<C, R, V>::line_length(line<C> const * const l)
{
  if (l == nullptr) return 0;
  else return l->length;
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::put(const uint8_t c)
{
  editor::cursor& cur = this->cursor;
  line<C> * l = mutref(this->lines[cur.row]);

  //   v
  // 0123
  if (cur.col >= C)
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

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::delete_forward()
{
  if (this->mode == mode::mark) {
    this->mode = mode::normal;
    return mark_delete();
  }

  editor::cursor& cur = this->cursor;

  if (line_length(this->lines[cur.row]) == 0) {
    if (this->length == 1) return false;
    decref(this->lines[cur.row]);
    // shift all lines up by 1
    int32_t n_lines = this->length - (cur.row + 1);
    move(&this->lines[cur.row],
	 &this->lines[cur.row+1],
	 (sizeof (line<C>*)) * n_lines);
    this->length--;
    this->lines[this->length] = nullptr;
    // no scrolling needed--cursor is already in the correct position
  } else {
    //    c
    // 01234
    line<C> * l = mutref(this->lines[cur.row]);
    int32_t length = l->length - cur.col;
    move(&l->buf[cur.col], &l->buf[cur.col+1], length);
    l->length--;

    // no scrolling needed--cursor is already in the correct position
  }

  return true;
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::delete_backward()
{
  if (this->mode == mode::mark) {
    this->mode = mode::normal;
    return mark_delete();
  }

  editor::cursor& cur = this->cursor;

  if (cur.col == 0) {
    if (cur.row == 0) return false;
    // make selection
    editor::cursor min { line_length(this->lines[cur.row-1]), cur.row-1 };
    editor::cursor max { cur.col, cur.row };
    selection sel { &min, &max };
    selection_delete(sel);

    cur.row--;
    scroll_up();
    scroll_new_col(min.col);
  } else {
    //    c
    // 01234
    line<C> * l = mutref(this->lines[cur.row]);
    int32_t length = l->length - cur.col;
    move(&l->buf[cur.col-1], &l->buf[cur.col], length);
    l->length--;

    cur.col--;
    scroll_left();
  }

  return true;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::delete_word_forward()
{
  mark_set();
  cursor_scan_word_forward();
  const selection sel = mark_get();
  selection_delete(sel);

  // move cur to sel.min
  scroll_new_cursor(*sel.min);

  this->mode = mode::normal;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::delete_word_backward()
{
  mark_set();
  cursor_scan_word_backward();
  const selection sel = mark_get();
  selection_delete(sel);

  // move cur to sel.min
  scroll_new_cursor(*sel.min);

  this->mode = mode::normal;
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::cursor_left()
{
  editor::cursor& cur = this->cursor;

  if (cur.col <= 0) {
    if (cur.row <= 0)
      return false;

    cur.row--;
    scroll_up();

    line<C> const * const l = this->lines[cur.row];
    cur.col = line_length(l);
    scroll_right();
  } else {
    cur.col--;
    scroll_left();
  }

  return true;
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::cursor_right()
{
  editor::cursor& cur = this->cursor;

  line<C> const * const l = this->lines[cur.row];
  int32_t length = line_length(l);
  if (cur.col >= length) {
    if (cur.row >= (this->length - 1))
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

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::cursor_up()
{
  editor::cursor& cur = this->cursor;

  if (cur.row <= 0)
    return false;

  cur.row--;
  scroll_up();

  line<C> const * const l = this->lines[cur.row];
  cur.col = min(cur.col, line_length(l));
  scroll_left();
  return true;
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::cursor_down()
{
  editor::cursor& cur = this->cursor;

  if (cur.row >= (this->length - 1))
    return false;

  cur.row++;
  scroll_down();

  line<C> const * const l = this->lines[cur.row];
  cur.col = min(cur.col, line_length(l));
  scroll_left();
  return true;
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::cursor_home()
{
  editor::cursor& cur = this->cursor;
  cur.col = 0;
  scroll_left();
  return true;
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::cursor_end()
{
  editor::cursor& cur = this->cursor;

  cur.col = line_length(this->lines[cur.row]);
  scroll_right();
  return true;
}

static inline constexpr bool word_boundary(int8_t c)
{
  return ((c >= 'a' && c <= 'z')
       || (c >= 'A' && c <= 'Z')
       || (c >= '0' && c <= '9'));
}

template <int C, int R, int V>
inline constexpr uint8_t buffer<C, R, V>::cursor_get(const editor::cursor& cur)
{
  return this->lines[cur.row] == nullptr ? 0 :
    (cur.col == this->lines[cur.row]->length ? '\n' :
     this->lines[cur.row]->buf[cur.col]);
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::cursor_increment(editor::cursor& cur)
{
  if (cur.col >= line_length(this->lines[cur.row])) {
    if (cur.row + 1 >= this->length) {
      return false;
    } else {
      cur.row++;
      cur.col = 0;
    }
  } else {
    cur.col++;
  }
  return true;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::cursor_scan_word_forward()
{
  // copy of this->cursor
  editor::cursor cur = this->cursor;

  // if we are not inside a word boundary, scan in `dir` direction until we are
  // inside a word boundary
  while (!word_boundary(cursor_get(cur))) {
    if (!cursor_increment(cur)) { scroll_new_cursor(cur); return; }
  }

  // scan in `dir` direction until we are not inside a word boundary
  while (word_boundary(cursor_get(cur))) {
    if (!cursor_increment(cur)) { scroll_new_cursor(cur); return; }
  }

  scroll_new_cursor(cur);
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::cursor_decrement(editor::cursor& cur)
{
  if (cur.col == 0) {
    if (cur.row - 1 < 0) {
      return false;
    } else {
      cur.row--;
      cur.col = this->lines[cur.row]->length;
    }
  } else {
    cur.col--;
  }
  return true;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::cursor_scan_word_backward()
{
  // copy of this->cursor
  editor::cursor cur = this->cursor;
  cursor_decrement(cur);
  // if we are not inside a word boundary, scan in `dir` direction until we are
  // inside a word boundary
  while (!word_boundary(cursor_get(cur))) {
    if (!cursor_decrement(cur)) { scroll_new_cursor(cur); return; }
  }

  // scan in `dir` direction until we are not inside a word boundary
  while (word_boundary(cursor_get(cur))) {
    if (!cursor_decrement(cur)) { scroll_new_cursor(cur); return; }
  }

  cursor_increment(cur);
  scroll_new_cursor(cur);
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::enter()
{
  editor::cursor& cur = this->cursor;

  if (cur.row >= R)
    return false;

  if ((this->length - 1) > cur.row) {
    // first, move all lines down one
    int32_t n_lines = this->length - (cur.row + 1);
    // don't care about nullptr here, this never dereferences
    move(&this->lines[cur.row+2], &this->lines[cur.row+1], (sizeof (line<C>*)) * n_lines);
  }
  // column-wise copy of the cursor position to the newly-created line
  line<C> * new_l = this->allocate();
  line<C> *& old_l = this->lines[cur.row];
  //   v
  // 01234 (5)
  if (old_l != nullptr) {
    new_l->length = old_l->length - cur.col;
    if (new_l->length > 0) {
      old_l = mutref(old_l);
      old_l->length -= new_l->length;
      copy(&new_l->buf[0], &old_l->buf[cur.col], new_l->length);
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

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::line_kill()
{
  editor::cursor& cur = this->cursor;

  if (line_length(this->lines[cur.row]) == 0) {
    decref(this->lines[cur.row]);

    // shift all lines up by one
    int32_t n_lines = this->length - (cur.row + 1);
    move(&this->lines[cur.row],
	 &this->lines[cur.row+1],
	 (sizeof (line<C>*)) * n_lines);
    this->length--;
    this->lines[this->length] = nullptr;
  } else {
    line<C> * l = mutref(this->lines[cur.row]);
    l->length = cur.col;
  }
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::mark_set()
{
  this->mark.row = this->cursor.row;
  this->mark.col = this->cursor.col;
  this->mode = mode::mark;
}

template <int C, int R, int V>
inline constexpr selection buffer<C, R, V>::mark_get()
{
  editor::cursor& cur = this->cursor;
  editor::cursor& mark = this->mark;
  selection sel;

  if (cur.row == mark.row) {
    if (cur.col == mark.col) {
      sel.min = sel.max = &cur;
    } else if (cur.col < mark.col) {
      sel.min = &cur;
      sel.max = &mark;
    } else {
      sel.min = &mark;
      sel.max = &cur;
    }
  } else if (cur.row < mark.row) {
    sel.min = &cur;
    sel.max = &mark;
  } else {
    sel.min = &mark;
    sel.max = &cur;
  }
  return sel;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::delete_from_line(line<C> *& l,
						     const int32_t col_start_ix,
						     const int32_t col_end_ix)
{
  if (l == nullptr) return;
  else if (col_start_ix == 0 && col_end_ix == l->length)
    decref(l);
  else if (col_end_ix == l->length)
    mutref(l)->length = col_start_ix;
  else {
    //   S  E
    // 0123456(7)
    // 0156
    l = mutref(l);
    int32_t length = l->length - col_end_ix;
    move(&l->buf[col_start_ix], &l->buf[col_end_ix], length);
    l->length = col_start_ix + length;
  }
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::selection_delete(const selection& sel)
{
  if (sel.min == sel.max) {
    return;
  } else if (sel.min->row == sel.max->row) {
    // delete from min.col to max.col (excluding max.col)
    delete_from_line(this->lines[sel.min->row], sel.min->col, sel.max->col);
  } else {
    // decref all lines between min.row and max.row, exclusive
    for (int32_t ix = (sel.min->row + 1); ix < (sel.max->row); ix++) {
      decref(this->lines[ix]);
    }

    // remove sel.min->col from lmin
    line<C> * lmin = mutref(this->lines[sel.min->row]);
    lmin->length -= (lmin->length - sel.min->col);

    if (this->lines[sel.max->row] != nullptr) {
      // combine min/max; truncating overflow
      const line<C> * lmax = this->lines[sel.max->row];

      //  v
      // 0123
      //   v
      // abcd(4)

      // 0cd

      int32_t move_length = min(lmax->length - sel.max->col, C - lmin->length);
      move(&lmin->buf[sel.min->col],
	   &lmax->buf[sel.max->col],
	   move_length);
      lmin->length += move_length;

      // delete max
      decref(this->lines[sel.max->row]);
    }

    // shift rows up
    int32_t n_lines = this->length - (sel.max->row + 1);
    move(&this->lines[sel.min->row + 1],
	 &this->lines[sel.max->row + 1],
	 (sizeof (line<C>*)) * n_lines);

    int32_t deleted_rows = (sel.max->row - sel.min->row);

    // don't decref here -- the references were just moved
    for (int32_t ix = this->length - deleted_rows; ix < this->length; ix++) {
      this->lines[ix] = nullptr;
    }

    this->length -= deleted_rows;
  }
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::mark_delete()
{
  const selection sel = mark_get();
  selection_delete(sel);

  // move cur to sel.min
  scroll_new_cursor(*sel.min);

  return true;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::quit()
{
  this->mode = mode::normal;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::shadow_clear()
{
  for (int32_t i = 0; i < this->shadow.length; i++)
    decref(this->shadow.lines[i]);

  this->shadow.length = 1;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::_shadow_cow(line<C> * src,
						const int32_t dst_row_ix,
						const int32_t col_start_ix,
						const int32_t col_end_ix)
{
  if (src == nullptr || (col_start_ix == col_end_ix)) {
    this->shadow.lines[dst_row_ix] = nullptr;
  } else if (col_start_ix == 0 && col_end_ix == src->length) {
    this->shadow.lines[dst_row_ix] = incref(src);
  } else {
    line<C> * dst = this->allocate();
    dst->length = col_end_ix - col_start_ix;
    copy(&dst->buf[0], &src->buf[col_start_ix], dst->length);
    this->shadow.lines[dst_row_ix] = dst;
  }
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::shadow_copy()
{
  if (this->mode != mode::mark)
    return false;

  this->shadow_clear();

  selection sel = this->mark_get();

  this->shadow.length = (sel.max->row - sel.min->row) + 1;

  if (sel.min == sel.max) {
    this->shadow.lines[0] = nullptr;
    return true;
  } else if (sel.min->row == sel.max->row) {
    // copy from min.col to max.col (excluding max.col)
    _shadow_cow(this->lines[sel.min->row],
		0,             // dst_row_ix
		sel.min->col,  // col_start_ix
		sel.max->col); // col_end_ix
  } else {
    line<C> * src = this->lines[sel.min->row];
    // copy from min.col to the end of the line (including min.col)
    _shadow_cow(src,
		0,                 // dst_row_ix
		sel.min->col,      // col_start_ix
		line_length(src)); // col_end_ix

    int32_t shadow_ix = 1;
    for (int32_t ix = (sel.min->row + 1); ix < (sel.max->row); ix++) {
      // cow all lines between min.row and max.row, exclusive
      this->shadow.lines[shadow_ix++] = incref(this->lines[ix]);
    }

    // copy from the beginning of the line to max.col (excluding max.col)
    _shadow_cow(this->lines[sel.max->row],
		shadow_ix,     // row_ix
		0,             // col_start_ix
		sel.max->col); // col_end_ix
  }

  this->mode = mode::normal;

  return true;
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::shadow_cut()
{
  if (this->mode != mode::mark)
    return false;

  // copy unsets mode::mark
  this->shadow_copy();

  this->mark_delete();

  return true;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::overwrite_line(line<C> *& dst,
						   const int32_t dst_col,
						   line<C> * src,
						   const int32_t src_col)
{
  if (src == nullptr) {
    if (dst_col == 0) dst = nullptr;
    //else do nothing
  } else if (dst_col == 0 && src_col == 0 &&
	     line_length(src) >= line_length(dst)) {
    decref(dst);
    dst = incref(src);
  } else {
    line<C> * dstmut = mutref(dst);
    int32_t copy_length = min(src->length - src_col, C - dst_col);
    move(&dstmut->buf[dst_col],
	 &src->buf[src_col],
	 copy_length);

    //   v
    // 012345
    // 2 + 4 = 6
    dstmut->length = max(dstmut->length, dst_col + copy_length);
  }
}

template <int C, int R, int V>
inline constexpr bool buffer<C, R, V>::shadow_paste()
{
  if (this->mode == mode::mark)
    this->mode = mode::normal;

  editor::cursor& cur = this->cursor;

  if (this->shadow.length == 1) {

    line<C> * ls = this->shadow.lines[0];
    line<C> *& lc = this->lines[cur.row];
    //             dst, dst_col
    overwrite_line(lc,  cur.col + ls->length, // (dst_col = 2 + 3 = 5)
    //             src, src_col
		   lc,  cur.col); // (length = 5 - 2 = 3)
    // now copy shadow to lc
    overwrite_line(lc, cur.col,
		   ls, 0);

    cur.col += ls->length;
    scroll_right();
  } else {
    // (qw
    //  er
    //  gh)
    //
    // aBcd
    // zyx (1)
    //
    // aqw
    // er (cow)
    // ghBcd
    // zyx (3)

    int32_t last_line_offset = (this->shadow.length - 1); // (2)

    line<C> *& first_line_dst = this->lines[cur.row];
    line<C> * first_line_src = this->shadow.lines[0];
    line<C> *& last_line_dst = this->lines[cur.row + last_line_offset];
    line<C> * last_line_src = this->shadow.lines[last_line_offset];

    int32_t shift_first_ix = cur.row + 1;
    int32_t shift_lines = this->length - shift_first_ix; // (2 - 1)

    move(&this->lines[cur.row + this->shadow.length], // 3
	 &this->lines[shift_first_ix], // 1
	 (sizeof (line<C>*)) * shift_lines);

    // cow all lines other than the first and last
    for (int ix = 1; ix < last_line_offset; ix++) { // ix < 2, max(ix) == 1
      this->lines[cur.row + ix] = incref(this->shadow.lines[ix]);
    }

    // copy 'bcd' to last_line_dst
    //             dst,            dst_col
    overwrite_line(last_line_dst,  line_length(last_line_src),
    //             src,            src_col
		   first_line_dst, cur.col);

    // shrink first_line_dst to '1'
    if (first_line_dst != nullptr) first_line_dst->length = cur.col;

    // copy 'qw' to first_line_dst
    overwrite_line(first_line_dst, cur.col,
		   first_line_src, 0);

    // copy 'gh' to last_line_dst
    overwrite_line(last_line_dst, 0,
		   last_line_src, 0);

    this->length += last_line_offset;

    cur.row += last_line_offset;
    scroll_down();

    int32_t col = line_length(last_line_src);
    scroll_new_col(col);
  }

  return true;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::scroll_up()
{
  if (this->cursor.row < this->window.top)
    this->window.top = this->cursor.row;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::scroll_down()
{
  // 0: a  -
  // 1: bv -

  // 0: a
  // 1: b  -
  // 2: cv -

  if (this->cursor.row >= this->window.top + this->window.cell_height)
    this->window.top = (this->cursor.row - (this->window.cell_height - 1));
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::scroll_left()
{
  if (this->cursor.col < this->window.left)
    this->window.left = this->cursor.col;
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::scroll_right()
{
  // 0: a  -
  // 1: bv -

  // 0: a
  // 1: b  -
  // 2: cv -

  if (this->cursor.col >= this->window.left + this->window.cell_width)
    this->window.left = (this->cursor.col - (this->window.cell_width - 1));
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::scroll_new_cursor(const editor::cursor& oth)
{
  editor::cursor& cur = this->cursor;

  if (oth.row < cur.row) { cur.row = oth.row; scroll_up();   }
  else                   { cur.row = oth.row; scroll_down(); }

  if (oth.col < cur.col) { cur.col = oth.col; scroll_right(); }
  else                   { cur.col = oth.col; scroll_left();  }
}

template <int C, int R, int V>
inline constexpr void buffer<C, R, V>::scroll_new_col(const int32_t col)
{
  editor::cursor& cur = this->cursor;

  if (col < cur.col) { cur.col = col; scroll_right(); }
  else               { cur.col = col; scroll_left();  }
}
}
