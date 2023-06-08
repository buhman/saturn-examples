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
  assert(l->buf[1] == 0x7f);
  assert(b.cursor.col == 1);
  assert(b.cursor.row == 0);
  assert(b.length == 1);

  b.put('b');
  l = b.lines[0];
  assert(l->length == 2);
  assert(l->buf[0] == 'a');
  assert(l->buf[1] == 'b');
  assert(l->buf[2] == 0x7f);
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
  assert(l->buf[3] == 0x7f);
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

  assert(b.backspace() == true);
  assert(l->length == 0);
  assert(l->buf[0] == 0x7f);

  assert(b.backspace() == false);

  b.put('b');
  b.put('c');
  b.put('d');
  b.put('e');
  b.cursor.col = 2;
  assert(l->length == 4);

  //"bcde"
  assert(b.backspace() == true);
  assert(l->buf[0] == 'b');
  assert(l->buf[1] == 'd');
  assert(l->buf[2] == 'e');
  assert(l->buf[3] == 0x7f);
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
  b.backspace();
  b.backspace();
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
  b.backspace();
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

  b.backspace();
  assert(b.length == 3);
  assert(b.lines[0]->buf[0] == 'a');
  assert(b.lines[1]->length == 0);
  assert(b.lines[2]->buf[0] == 'b');
  assert(b.lines[3] == nullptr);
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

  return 0;
}
