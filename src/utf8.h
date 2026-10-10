#ifndef LOMBRICO_UTF8_H
#define LOMBRICO_UTF8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UTF8_INVALID 0xfffdu

uint32_t utf8_decode(const char **text, const char *end);
size_t   utf8_encode(uint32_t glyph, char out[4]);
bool     utf8_narrow(uint32_t glyph);

#endif
