#ifndef LOMBRICO_PRESS_H
#define LOMBRICO_PRESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KEY_ESCAPE 0x1b

enum press_kind {
	PRESS_KEY,
	PRESS_CLICK,
};

struct press {
	enum press_kind kind;
	uint32_t        key;
	int             x;
	int             y;
};

size_t press_parse(const char *bytes, size_t len, struct press *, bool *got);

#endif
