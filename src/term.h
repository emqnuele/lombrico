#ifndef LOMBRICO_TERM_H
#define LOMBRICO_TERM_H

#include <stdbool.h>
#include <stddef.h>

#include "press.h"

int  term_open(void);
void term_close(void);
void term_size(int *width, int *height);
bool term_resized(void);
bool term_scrambled(void);
bool term_quitting(void);
bool term_press(struct press *);
void term_write(const char *bytes, size_t len);

#endif
