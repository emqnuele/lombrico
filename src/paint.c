#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "paint.h"
#include "term.h"
#include "utf8.h"

#define SYNC_BEGIN "\x1b[?2026h"
#define SYNC_END   "\x1b[?2026l"
#define CLEAR      "\x1b[0m\x1b[2J"

#define CELL_BYTES 40

static struct cell *shown;
static int shown_width, shown_height;

static char *out;
static size_t out_len, out_cap;

static void
put(const char *bytes, size_t len)
{
	if (out_len + len > out_cap) {
		return;
	}
	memcpy(out + out_len, bytes, len);
	out_len += len;
}

static void
put_uint(unsigned value)
{
	char digits[10];
	size_t n = 0;

	do {
		digits[sizeof digits - ++n] = (char)('0' + value % 10);
		value /= 10;
	} while (value > 0);

	put(digits + sizeof digits - n, n);
}

static void
put_jump(int x, int y)
{
	put("\x1b[", 2);
	put_uint((unsigned)y + 1);
	put(";", 1);
	put_uint((unsigned)x + 1);
	put("H", 1);
}

static void
put_ink(const struct cell *c)
{
	put("\x1b[38;2;", 7);
	put_uint(c->r);
	put(";", 1);
	put_uint(c->g);
	put(";", 1);
	put_uint(c->b);
	put("m", 1);
}

static void
put_glyph(uint32_t glyph)
{
	char bytes[4];

	put(bytes, utf8_encode(glyph, bytes));
}

static bool
same_ink(const struct cell *a, const struct cell *b)
{
	return a->r == b->r && a->g == b->g && a->b == b->b;
}

static bool
same_cell(const struct cell *a, const struct cell *b)
{
	return a->glyph == b->glyph && (a->glyph == ' ' || same_ink(a, b));
}

static int
fit(int width, int height)
{
	size_t area = (size_t)width * (size_t)height;
	struct cell *cells;
	char *bytes;

	cells = realloc(shown, area * sizeof *cells);
	if (cells == NULL) {
		return -1;
	}
	shown = cells;

	bytes = realloc(out, area * CELL_BYTES + 64);
	if (bytes == NULL) {
		return -1;
	}
	out = bytes;
	out_cap = area * CELL_BYTES + 64;

	shown_width = width;
	shown_height = height;

	return 0;
}

int
paint(const struct canvas *c)
{
	const struct cell *now, *was;
	struct cell pen = { 0 };
	bool inked = false, full;
	int x, y, cx = -1, cy = -1;

	full = term_scrambled();
	if (c->width != shown_width || c->height != shown_height) {
		if (fit(c->width, c->height) < 0) {
			return -1;
		}
		full = true;
	}

	out_len = 0;
	put(SYNC_BEGIN, sizeof SYNC_BEGIN - 1);
	if (full) {
		put(CLEAR, sizeof CLEAR - 1);
	}

	for (y = 0; y < c->height; y++) {
		for (x = 0; x < c->width; x++) {
			now = &c->cells[y * c->width + x];
			was = &shown[y * c->width + x];

			if (full ? now->glyph == ' ' : same_cell(now, was)) {
				continue;
			}
			if (x != cx || y != cy) {
				put_jump(x, y);
			}
			if (now->glyph != ' ' && (!inked || !same_ink(now, &pen))) {
				put_ink(now);
				pen = *now;
				inked = true;
			}
			put_glyph(now->glyph);
			cx = x + 1;
			cy = y;
		}
	}

	put(SYNC_END, sizeof SYNC_END - 1);
	memcpy(shown, c->cells,
	    (size_t)c->width * (size_t)c->height * sizeof *shown);
	term_write(out, out_len);

	return 0;
}

void
paint_free(void)
{
	free(shown);
	free(out);
	shown = NULL;
	out = NULL;
	shown_width = shown_height = 0;
	out_len = out_cap = 0;
}
