#include "serial.h"

#include "sh2.h"

void serial_char(const char c)
{
  //while ((sh2.reg.SSR & SSR__TDRE) == 0); // wait for transmit data empty
  sh2.reg.TDR = c;
}

void serial_string(const char * s)
{
  while (*s) {
    //while ((sh2.reg.SSR & SSR__TDRE) == 0); // wait for transmit data empty
    sh2.reg.TDR = *s++;
  }
}

void serial_bytes(const char * s, uint32_t length)
{
  while (length > 0) {
    //while ((sh2.reg.SSR & SSR__TDRE) == 0); // wait for transmit data empty
    sh2.reg.TDR = *s++;
    length -= 1;
  }
}

static void hex(char * c, uint32_t len, uint32_t n)
{
  while (len > 0) {
    uint32_t nib = n & 0xf;
    n = n >> 4;
    if (nib > 9) {
      nib += (97 - 10);
    } else {
      nib += (48 - 0);
    }

    c[--len] = nib;
  }
}

void serial_integer(const uint32_t n, const uint32_t length, const char end)
{
  char num_buf[length];
  hex(num_buf, length, n);
  serial_string("0x");
  serial_bytes(num_buf, length);
  serial_char(end);
}
