#include "press.h"
#include "utf8.h"

#define BUTTONS 3
#define MOTION  32
#define WHEEL   64
#define LARGEST 100000

static bool
final(char c)
{
	return c >= 0x40 && c <= 0x7e;
}

static size_t
expected(unsigned char lead)
{
	if ((lead & 0xe0) == 0xc0) {
		return 2;
	}
	if ((lead & 0xf0) == 0xe0) {
		return 3;
	}
	if ((lead & 0xf8) == 0xf0) {
		return 4;
	}
	return 1;
}

static const char *
number(const char *p, const char *end, int *value)
{
	*value = 0;
	while (p < end && *p >= '0' && *p <= '9') {
		if (*value < LARGEST) {
			*value = *value * 10 + (*p - '0');
		}
		p++;
	}
	return p;
}

static size_t
mouse(const char *bytes, size_t len, struct press *out, bool *got)
{
	const char *p, *end;
	int button, x, y;
	size_t i;

	for (i = 3; i < len && !final(bytes[i]); i++) {
	}
	if (i == len) {
		return 0;
	}

	end = bytes + i;
	p = number(bytes + 3, end, &button);
	if (p == end || *p != ';') {
		return i + 1;
	}
	p = number(p + 1, end, &x);
	if (p == end || *p != ';') {
		return i + 1;
	}
	number(p + 1, end, &y);

	if (bytes[i] == 'M' && (button & BUTTONS) == 0 &&
	    (button & (MOTION | WHEEL)) == 0 && x > 0 && y > 0) {
		*out = (struct press){ .kind = PRESS_CLICK, .x = x - 1, .y = y - 1 };
		*got = true;
	}
	return i + 1;
}

size_t
press_parse(const char *bytes, size_t len, struct press *out, bool *got)
{
	const char *p = bytes;
	size_t i;

	*got = false;
	if (len == 0) {
		return 0;
	}

	if (bytes[0] != KEY_ESCAPE) {
		if (expected((unsigned char)bytes[0]) > len) {
			return 0;
		}
		*out = (struct press){
			.kind = PRESS_KEY,
			.key = utf8_decode(&p, bytes + len),
		};
		*got = true;
		return (size_t)(p - bytes);
	}

	if (len == 1) {
		*out = (struct press){ .kind = PRESS_KEY, .key = KEY_ESCAPE };
		*got = true;
		return 1;
	}

	if (bytes[1] == '[') {
		if (len == 2) {
			return 0;
		}
		if (bytes[2] == '<') {
			return mouse(bytes, len, out, got);
		}
		for (i = 2; i < len && !final(bytes[i]); i++) {
		}
		return i < len ? i + 1 : 0;
	}
	if (bytes[1] == 'O') {
		return len >= 3 ? 3 : 0;
	}

	return 2;
}
