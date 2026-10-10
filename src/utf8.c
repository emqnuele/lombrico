#include "utf8.h"

uint32_t
utf8_decode(const char **text, const char *end)
{
	const unsigned char *s = (const unsigned char *)*text;
	size_t left = (size_t)(end - *text);
	uint32_t glyph, least;
	size_t need, i;

	if (s[0] < 0x80) {
		*text += 1;
		return s[0];
	}

	if ((s[0] & 0xe0) == 0xc0) {
		glyph = s[0] & 0x1f;
		need = 1;
		least = 0x80;
	} else if ((s[0] & 0xf0) == 0xe0) {
		glyph = s[0] & 0x0f;
		need = 2;
		least = 0x800;
	} else if ((s[0] & 0xf8) == 0xf0) {
		glyph = s[0] & 0x07;
		need = 3;
		least = 0x10000;
	} else {
		*text += 1;
		return UTF8_INVALID;
	}

	if (need >= left) {
		*text += 1;
		return UTF8_INVALID;
	}

	for (i = 1; i <= need; i++) {
		if ((s[i] & 0xc0) != 0x80) {
			*text += 1;
			return UTF8_INVALID;
		}
		glyph = (glyph << 6) | (s[i] & 0x3f);
	}

	*text += need + 1;

	if (glyph < least || glyph > 0x10ffff ||
	    (glyph >= 0xd800 && glyph <= 0xdfff)) {
		return UTF8_INVALID;
	}

	return glyph;
}

size_t
utf8_encode(uint32_t glyph, char out[4])
{
	if (glyph < 0x80) {
		out[0] = (char)glyph;
		return 1;
	}
	if (glyph < 0x800) {
		out[0] = (char)(0xc0 | (glyph >> 6));
		out[1] = (char)(0x80 | (glyph & 0x3f));
		return 2;
	}
	if (glyph < 0x10000) {
		out[0] = (char)(0xe0 | (glyph >> 12));
		out[1] = (char)(0x80 | ((glyph >> 6) & 0x3f));
		out[2] = (char)(0x80 | (glyph & 0x3f));
		return 3;
	}
	out[0] = (char)(0xf0 | (glyph >> 18));
	out[1] = (char)(0x80 | ((glyph >> 12) & 0x3f));
	out[2] = (char)(0x80 | ((glyph >> 6) & 0x3f));
	out[3] = (char)(0x80 | (glyph & 0x3f));
	return 4;
}

bool
utf8_narrow(uint32_t glyph)
{
	return (glyph >= 0x20 && glyph < 0x7f) ||
	    (glyph >= 0xa0 && glyph < 0x250) ||
	    (glyph >= 0x2190 && glyph < 0x2194) ||
	    (glyph >= 0x2500 && glyph < 0x2580) ||
	    glyph == 0x2663 || glyph == 0x2665 ||
	    glyph == 0x266a || glyph == 0x266b;
}
