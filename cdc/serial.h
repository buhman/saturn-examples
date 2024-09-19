#include <stdint.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void serial_char(const char c);
void serial_string(const char * s);
void serial_bytes(const char * s, uint32_t length);
void serial_integer(const uint32_t n, const uint32_t length, const char end);

#ifdef __cplusplus
}
#endif
