#ifndef LOMBRICO_STREAM_H
#define LOMBRICO_STREAM_H

#include <stdbool.h>
#include <stddef.h>

#define STREAM_ENDED (-1)

bool stream_fed(void);
bool stream_piped_out(void);
long stream_read(char *bytes, size_t cap);
bool stream_pass(const char *bytes, size_t len);

#endif
