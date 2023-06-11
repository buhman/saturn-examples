#include <assert.h>
#include <iostream>

#include "editor.hpp"

using namespace editor;

static void test_allocate()
{
  buffer<8, 4> b {4, 2};
  decltype(b)::line_type * l;

  assert(b.row[0].length == -1);
  assert(b.row[1].length == -1);
  assert(b.row[2].length == -1);
  assert(b.row[3].length == -1);

  l = b.allocate();
  assert(b.row[0].length == 0);
  assert(b.row[1].length == -1);
  assert(b.row[2].length == -1);
  assert(b.row[3].length == -1);
  assert(l == &b.row[0]);

  l = b.allocate();
  assert(b.row[0].length == 0);
  assert(b.row[1].length == 0);
  assert(b.row[2].length == -1);
  assert(b.row[3].length == -1);
  assert(l == &b.row[1]);

  l = b.allocate();
  assert(b.row[0].length == 0);
  assert(b.row[1].length == 0);
  assert(b.row[2].length == 0);
  assert(b.row[3].length == -1);
  assert(l == &b.row[2]);

  l = b.allocate();
  assert(b.row[0].length == 0);
  assert(b.row[1].length == 0);
  assert(b.row[2].length == 0);
  assert(b.row[3].length == 0);
  assert(l == &b.row[3]);

  l = &b.row[1];
  b.deallocate(l);
  assert(b.row[0].length == 0);
  assert(b.row[1].length == -1);
  assert(b.row[2].length == 0);
  assert(b.row[3].length == 0);

  l = b.allocate();
  assert(b.row[0].length == 0);
  assert(b.row[1].length == 0);
  assert(b.row[2].length == 0);
  assert(b.row[3].length == 0);
  assert(l == &b.row[1]);
}

static void test_put()
{
  //   v
  // "as" -> "abs"
  buffer<8, 4> b {4, 2};
  decltype(b)::line_type * l;

  assert(b.cursor.col == 0);
  assert(b.cursor.row == 0);
  assert(b.length == 1);

  b.put('a');
  l = b.lines[0];
  assert(l = &b.row[0]);
  assert(l->length == 1);
  assert(l->buf[0] == 'a');
  assert(b.cursor.col == 1);
  assert(b.cursor.row == 0);
  assert(b.length == 1);

  b.put('b');
  l = b.lines[0];
  assert(l->length == 2);
  assert(l->buf[0] == 'a');
  assert(l->buf[1] == 'b');
  assert(b.cursor.col == 2);
  assert(b.cursor.row == 0);
  assert(b.length == 1);

  b.cursor.col = 1;
  b.put('c');
  l = b.lines[0];
  assert(l->length == 3);
  assert(l->buf[0] == 'a');
  assert(l->buf[1] == 'c');
  assert(l->buf[2] == 'b');
  assert(b.cursor.col == 2);
  assert(b.cursor.row == 0);
  assert(b.length == 1);
}

void test_backspace()
{
  buffer<8, 4> b {4, 2};
  decltype(b)::line_type * l;

  b.put('a');
  l = b.lines[0];
  assert(l->length == 1);
  assert(l->buf[0] == 'a');

  assert(b.delete_backward() == true);
  assert(l->length == 0);

  assert(b.delete_backward() == false);

  b.put('b');
  b.put('c');
  b.put('d');
  b.put('e');
  b.cursor.col = 2;
  assert(l->length == 4);

  //"bcde"
  assert(b.delete_backward() == true);
  assert(l->buf[0] == 'b');
  assert(l->buf[1] == 'd');
  assert(l->buf[2] == 'e');
  assert(l->length == 3);
}

void test_enter()
{
  // [0] asDf
  // [1] qwer

  // [0] as
  // [1] Df
  // [2] qwer
  buffer<8, 4> b {4, 2};

  b.cursor.row = 0;
  b.cursor.col = 0;
  b.put('a');
  b.put('s');
  b.put('d');
  b.put('f');
  b.cursor.row = 1;
  b.cursor.col = 0;
  b.put('q');
  b.put('w');
  b.put('e');
  b.put('r');

  assert(b.length == 2);
  assert(b.lines[0]->buf[0] == 'a');
  assert(b.lines[0]->buf[3] == 'f');
  assert(b.lines[1]->buf[0] == 'q');
  assert(b.lines[1]->buf[3] == 'r');

  b.cursor.row = 0;
  b.cursor.col = 2;
  b.enter();

  assert(b.length == 3);
  assert(b.lines[0]->length == 2);
  assert(b.lines[1]->length == 2);
  assert(b.lines[2]->length == 4);
  assert(b.lines[0]->buf[0] == 'a');
  assert(b.lines[0]->buf[1] == 's');

  assert(b.lines[1]->buf[0] == 'd');
  assert(b.lines[1]->buf[1] == 'f');

  assert(b.lines[2]->buf[0] == 'q');
  assert(b.lines[2]->buf[1] == 'w');
  assert(b.lines[2]->buf[2] == 'e');
  assert(b.lines[2]->buf[3] == 'r');
}

void test_enter_backspace1()
{
  // abcd

  // ab
  //
  // cd

  // abcd

  // ab
  // cd

  buffer<8, 4> b {4, 2};

  b.put('a');
  b.put('b');
  b.put('c');
  b.put('d');
  b.cursor_left();
  b.cursor_left();
  b.enter();
  b.enter();
  assert(b.length == 3);
  b.delete_backward();
  b.delete_backward();
  assert(b.length == 1);
  b.enter();
  assert(b.length == 2);

  assert(b.lines[0]->buf[0] == 'a');
  assert(b.lines[0]->buf[1] == 'b');
  assert(b.lines[1]->buf[0] == 'c');
  assert(b.lines[1]->buf[1] == 'd');
}

void test_enter_backspace2()
{
  buffer<8, 4> b {4, 2};

  b.put('a');
  b.enter();
  assert(b.cursor.row == 1);
  assert(b.cursor.col == 0);
  assert(b.length == 2);
  b.delete_backward();
  assert(b.cursor.row == 0);
  assert(b.cursor.col == 1);
  assert(b.length == 1);
}

void test_enter_scroll()
{
  buffer<8, 4> b {4, 2};

  assert(b.window.top == 0);
  b.put('a');
  assert(b.window.top == 0);
  b.enter();
  assert(b.window.top == 0);
  b.put('b');
  assert(b.window.top == 0);
  b.enter();
  assert(b.cursor.row == 2);
  assert(b.window.top == 1);
  b.put('c');
  assert(b.window.top == 1);
  b.enter();
  assert(b.window.top == 2);
  b.put('d');
  assert(b.window.top == 2);
}

void test_first_enter()
{
  buffer<8, 4> b {4, 2};

  b.enter();
  assert(b.length == 2);
}

void test_enter_backspace3()
{
  // a
  //
  //
  // b

  buffer<8, 8> b {4, 2};

  b.put('a');
  b.enter();
  b.enter();
  b.enter();
  b.put('b');
  b.cursor_up();
  assert(b.lines[0]->buf[0] == 'a');
  assert(b.lines[1]->length == 0);
  assert(b.lines[2]->length == 0);
  assert(b.lines[3]->buf[0] == 'b');
  assert(b.lines[4] == nullptr);
  assert(b.length == 4);

  b.delete_backward();
  assert(b.length == 3);
  assert(b.lines[0]->buf[0] == 'a');
  assert(b.lines[1]->length == 0);
  assert(b.lines[2]->buf[0] == 'b');
  assert(b.lines[3] == nullptr);
}

void test_copy()
{
  // 0123

  // asDf
  // qwer
  // jKlo

  // asklo
  //
  // df
  // qwer
  // j

  buffer<8, 8> b {4, 2};

  b.put('a');
  b.put('s');
  b.put('d');
  b.put('f');
  b.enter();
  b.put('q');
  b.put('w');
  b.put('e');
  b.put('r');
  b.enter();
  b.put('j');
  b.put('k');
  b.put('l');
  b.put('o');

  b.cursor_up();
  b.cursor_up();
  b.cursor_left();
  b.cursor_left();
  b.mark_set();
  b.cursor_down();
  b.cursor_down();
  b.cursor_left();

  assert(b.mark.row == 0);
  assert(b.mark.col == 2);
  assert(b.cursor.row == 2);
  assert(b.cursor.col == 1);

  b.shadow_copy();
  assert(b.shadow.length == 3);
  assert(b.shadow.lines[0] != nullptr);
  assert(b.shadow.lines[1] != nullptr);
  assert(b.shadow.lines[2] != nullptr);
  assert(b.shadow.lines[3] == nullptr);
  assert(b.shadow.lines[0]->length == 2);
  assert(b.shadow.lines[1]->length == 4);
  assert(b.shadow.lines[2]->length == 1);
  assert(b.shadow.lines[0]->buf[0] == 'd');
  assert(b.shadow.lines[0]->buf[1] == 'f');
  assert(b.shadow.lines[1]->buf[0] == 'q');
  assert(b.shadow.lines[1]->buf[1] == 'w');
  assert(b.shadow.lines[1]->buf[2] == 'e');
  assert(b.shadow.lines[1]->buf[3] == 'r');
  assert(b.shadow.lines[2]->buf[0] == 'j');
  assert(b.shadow.lines[0] != b.lines[0]);
  assert(b.shadow.lines[1] == b.lines[1]);
  assert(b.shadow.lines[2] != b.lines[2]);
}

void test_copy_same_line()
{
  buffer<8, 8> b {4, 2};

  //  v
  // asdF
  //
  // sd

  b.put('a');
  b.put('s');
  b.put('d');
  b.put('f');
  b.cursor_left();
  b.mark_set();
  b.cursor_left();
  b.cursor_left();

  // non-cow same line
  b.shadow_copy();
  assert(b.shadow.length == 1);
  assert(b.shadow.lines[0] != nullptr);
  assert(b.shadow.lines[1] == nullptr);
  assert(b.shadow.lines[0] != b.lines[0]);
  assert(b.shadow.lines[0]->length == 2);
  assert(b.shadow.lines[0]->buf[0] == 's');
  assert(b.shadow.lines[0]->buf[1] == 'd');

  // cow same line
  b.cursor_home();
  b.mark_set();
  b.cursor_end();
  b.shadow_copy();
  assert(b.shadow.length == 1);
  assert(b.shadow.lines[0] == b.lines[0]);
}

void test_copy_multi_line_cow()
{
  buffer<8, 8> b {4, 2};

  //  v
  // asdF
  //
  // sd

  b.put('a');
  b.put('s');
  b.put('d');
  b.put('f');
  b.enter();
  b.put('q');
  b.put('w');
  b.put('e');
  b.put('r');
  b.mark_set();
  b.cursor_up();
  b.cursor_home();
  b.shadow_copy();

  assert(b.shadow.length == 2);
  assert(b.shadow.lines[0] == b.lines[0]);
  assert(b.shadow.lines[1] == b.lines[1]);
}

void test_copy_multi_line_offset()
{
  buffer<8, 8> b {4, 2};

  b.put('a');
  b.put('s');
  b.put('d');
  b.put('f');
  b.enter();
  b.put('q');
  b.put('w');
  b.put('e');
  b.put('r');
  b.enter();
  b.put('s');
  b.put('h');
  b.put('a');
  b.put('d');
  b.enter();
  b.put('o');
  b.put('w');

  // qwEr
  // sHad

  // er
  // s
  b.cursor_up();
  b.cursor_home();
  b.cursor_right();
  b.mark_set();
  b.cursor_up();
  b.cursor_right();
  b.shadow_copy();

  assert(b.shadow.length == 2);
  assert(b.shadow.lines[0]->length == 2);
  assert(b.shadow.lines[1]->length == 1);
  assert(b.shadow.lines[0]->buf[0] == 'e');
  assert(b.shadow.lines[0]->buf[1] == 'r');
  assert(b.shadow.lines[1]->buf[0] == 's');

  b.cursor_home();
  b.mark_set();
  b.cursor_down();
  b.cursor_down();
  b.cursor_end();
  b.shadow_copy();

  assert(b.shadow.length == 3);
  assert(b.shadow.lines[0] == b.lines[1]);
  assert(b.shadow.lines[1] == b.lines[2]);
  assert(b.shadow.lines[2] == b.lines[3]);
}

void test_delete_from_line()
{
  buffer<8, 8> b {4, 2};

  b.put('a');
  b.put('s');
  b.put('d');
  b.put('f');

  //  S E
  // asdf
  // af

  b.delete_from_line(b.lines[0], 1, 3);
  assert(b.lines[0]->length == 2);
  assert(b.lines[0]->buf[0] == 'a');
  assert(b.lines[0]->buf[1] == 'f');

  b.delete_from_line(b.lines[0], 0, 2);
  assert(b.lines[0] == nullptr);
}

void test_selection_delete()
{
  buffer<8, 8> b {4, 2};

  b.put('s');
  b.put('p');
  b.put('a');
  b.put('m');
  b.enter();
  b.put('a');
  b.put('s');
  b.put('d');
  b.put('f');
  b.enter();
  b.put('0');
  b.put('1');
  b.put('2');
  b.put('3');
  b.enter();
  b.put('j');
  b.put('k');
  b.put('l');
  b.put('p');
  b.enter();
  b.put('q');
  b.put('w');
  b.put('e');
  b.put('r');

  // spam    (0)
  // asDf <  (1)
  // 0123 -- (2) (del)
  // jKlp <  (3) (del)
  // qwer    (4)
  // (5)

  // spam  (0)
  // asklp (1)
  // qwer  (2)

  cursor min { 2, 1 };
  cursor max { 1, 3 };
  selection sel { &min, &max };
  b.selection_delete(sel);

  assert(b.length == 3);
  assert(b.lines[0]->length == 4);
  assert(b.lines[0]->buf[0] == 's');
  assert(b.lines[0]->buf[1] == 'p');
  assert(b.lines[0]->buf[2] == 'a');
  assert(b.lines[0]->buf[3] == 'm');
  assert(b.lines[1]->length == 5);
  assert(b.lines[1]->buf[0] == 'a');
  assert(b.lines[1]->buf[1] == 's');
  assert(b.lines[1]->buf[2] == 'k');
  assert(b.lines[1]->buf[3] == 'l');
  assert(b.lines[1]->buf[4] == 'p');
  assert(b.lines[2]->length == 4);
  assert(b.lines[2]->buf[0] == 'q');
  assert(b.lines[2]->buf[1] == 'w');
  assert(b.lines[2]->buf[2] == 'e');
  assert(b.lines[2]->buf[3] == 'r');
}

void test_shadow_paste_oneline()
{
  buffer<8, 8> b {4, 2};

  b.put('q');
  b.put('w');
  b.put('r');
  b.mark_set();
  b.cursor_home();
  b.shadow_copy();
  b.mark_delete();
  assert(decltype(b)::line_length(b.lines[0]) == 0);
  assert(b.cursor.col == 0);

  b.put('a');
  b.put('b');
  b.put('c');
  b.put('d');
  b.put('e');

  b.cursor_left();
  b.cursor_left();
  b.cursor_left();

  // (qwr)
  // 01234567
  // abCde
  // abqwrCde

  assert(decltype(b)::line_length(b.lines[0]) == 5);
  b.shadow_paste();
  assert(decltype(b)::line_length(b.lines[0]) == 8);
  assert(b.lines[0]->buf[0] == 'a');
  assert(b.lines[0]->buf[1] == 'b');
  assert(b.lines[0]->buf[2] == 'q');
  assert(b.lines[0]->buf[3] == 'w');
  assert(b.lines[0]->buf[4] == 'r');
  assert(b.lines[0]->buf[5] == 'c');
  assert(b.lines[0]->buf[6] == 'd');
  assert(b.lines[0]->buf[7] == 'e');

  // also handles these other cases:
  // abc|
  // abcqwr

  // |
  // qwr (cow)
  b.decref(b.lines[0]);
  b.cursor_home();
  assert(b.shadow.lines[0]->buf[0] == 'q');
  assert(b.shadow.lines[0]->buf[1] == 'w');
  assert(b.shadow.lines[0]->buf[2] == 'r');

  b.put('a');
  b.put('b');
  b.put('c');
  assert(decltype(b)::line_length(b.lines[0]) == 3);
  b.shadow_paste();
  assert(decltype(b)::line_length(b.lines[0]) == 6);
  assert(b.lines[0]->buf[0] == 'a');
  assert(b.lines[0]->buf[1] == 'b');
  assert(b.lines[0]->buf[2] == 'c');
  assert(b.lines[0]->buf[3] == 'q');
  assert(b.lines[0]->buf[4] == 'w');
  assert(b.lines[0]->buf[5] == 'r');

  b.decref(b.lines[0]);
  b.cursor_home();

  b.shadow_paste();
  assert(b.lines[0] == b.shadow.lines[0]);
}

void test_shadow_paste_multiline()
{
  buffer<8, 8> b {4, 2};

  // (qw
  //  er
  //  gh)
  //
  // jklopr
  // aBcd
  // zyx (2)
  //
  // jklopr
  // aqw
  // er (cow)
  // ghBcd
  // zyx (4)

  b.put('q');
  b.put('w');
  b.enter();
  b.put('e');
  b.put('r');
  b.enter();
  b.put('g');
  b.put('h');
  b.mark_set();
  b.cursor_home();
  b.cursor_up();
  b.cursor_up();
  assert(b.cursor.row == 0);
  assert(b.cursor.col == 0);
  b.shadow_copy();
  b.mark_delete();
  assert(b.shadow.length == 3);
  assert(decltype(b)::line_length(b.shadow.lines[0]) == 2);
  assert(decltype(b)::line_length(b.shadow.lines[1]) == 2);
  assert(decltype(b)::line_length(b.shadow.lines[2]) == 2);
  assert(b.length == 1);
  assert(decltype(b)::line_length(b.lines[0]) == 0);

  // jkl
  // aBcd
  // zyx

  b.put('j');
  b.put('k');
  b.put('l');
  b.put('o');
  b.put('p');
  b.put('r');
  b.enter();
  b.put('a');
  b.put('b');
  b.put('c');
  b.put('d');
  b.enter();
  b.put('z');
  b.put('y');
  b.put('x');
  b.cursor_home();
  b.cursor_up();
  b.cursor_right();
  assert(b.cursor.row == 1);
  assert(b.cursor.col == 1);

  b.shadow_paste();

  // jklopr
  // aqw
  // er (cow)
  // ghBcd
  // zyx (4)

  assert(b.length == 5);
  assert(b.lines[0]->length == 6);
  assert(b.lines[1]->length == 3);
  assert(b.lines[2]->length == 2);
  assert(b.lines[3]->length == 5);
  assert(b.lines[4]->length == 3);

  assert(b.lines[1]->buf[0] == 'a');
  assert(b.lines[1]->buf[1] == 'q');
  assert(b.lines[1]->buf[2] == 'w');
  assert(b.lines[2] == b.shadow.lines[1]);
  assert(b.lines[2]->buf[0] == 'e');
  assert(b.lines[2]->buf[1] == 'r');
  assert(b.lines[3]->buf[0] == 'g');
  assert(b.lines[3]->buf[1] == 'h');
  assert(b.lines[3]->buf[2] == 'b');
  assert(b.lines[3]->buf[3] == 'c');
  assert(b.lines[3]->buf[4] == 'd');
  assert(b.lines[4]->buf[0] == 'z');
  assert(b.lines[4]->buf[1] == 'y');
  assert(b.lines[4]->buf[2] == 'x');
}

void test_delete_forward()
{
   buffer<8, 8> b {4, 2};
   b.put('a');
   b.put('b');
   b.put('c');
   b.cursor_left();
   b.cursor_left();
   b.delete_forward();
   assert(b.lines[0]->length == 2);
   assert(b.lines[0]->buf[0] == 'a');
   assert(b.lines[0]->buf[1] == 'c');

   //
   b.cursor_end();
   b.enter();
   b.enter();
   b.put('q');
   b.put('w');
   b.cursor_home();
   b.cursor_up();
   assert(b.length == 3);
   b.delete_forward();
   assert(b.length == 2);
   assert(b.lines[0]->length == 2);
   assert(b.lines[0]->buf[0] == 'a');
   assert(b.lines[0]->buf[1] == 'c');
   assert(b.lines[1]->length == 2);
   assert(b.lines[1]->buf[0] == 'q');
   assert(b.lines[1]->buf[1] == 'w');
}

int main()
{
  test_allocate();
  test_put();
  test_backspace();
  test_enter_backspace1();
  test_enter_backspace2();
  test_enter_backspace3();
  test_enter_scroll();
  test_first_enter();
  test_copy();
  test_copy_same_line();
  test_copy_multi_line_cow();
  test_copy_multi_line_offset();
  test_delete_from_line();
  test_selection_delete();
  test_shadow_paste_oneline();
  test_shadow_paste_multiline();
  test_delete_forward();

  return 0;
}
