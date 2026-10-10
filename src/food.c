#include <ctype.h>
#include <string.h>

#include "food.h"
#include "stream.h"
#include "utf8.h"

#define CHUNK        4096
#define FRAME_BUDGET (1L << 20)
#define TAB_STOP     4
#define LINES_PER_SECOND 15.0f
#define LINES_AT_ONCE    40.0f
#define FOOD_BACK    (-CANVAS_FRONT)

static const char *const spicy[] = {
	"error", "fail", "fatal", "panic", "exception",
};

void
food_open(struct food *f)
{
	memset(f, 0, sizeof *f);
	f->open = stream_fed();
	f->passing = f->open && stream_piped_out();
	f->appetite = LINES_AT_ONCE;
}

static const char *
skip_escape(const char *p, const char *end)
{
	char kind;

	if (++p >= end) {
		return end;
	}

	kind = *p++;
	if (kind == '[') {
		while (p < end && !(*p >= 0x40 && *p <= 0x7e)) {
			p++;
		}
		return p < end ? p + 1 : end;
	}
	if (kind == ']' || kind == 'P' || kind == '_' || kind == '^') {
		for (; p < end; p++) {
			if (*p == '\a') {
				return p + 1;
			}
			if (*p == '\x1b' && p + 1 < end && p[1] == '\\') {
				return p + 2;
			}
		}
		return end;
	}
	return p;
}

static int
clean(const char *p, const char *end, uint32_t *glyphs)
{
	uint32_t glyph;
	int width = 0;

	while (p < end && width < FOOD_WIDTH) {
		if (*p == '\x1b') {
			p = skip_escape(p, end);
			continue;
		}
		if (*p == '\t') {
			do {
				glyphs[width++] = ' ';
			} while (width % TAB_STOP != 0 && width < FOOD_WIDTH);
			p++;
			continue;
		}

		glyph = utf8_decode(&p, end);
		if (glyph < 0x20 || (glyph >= 0x7f && glyph < 0xa0)) {
			continue;
		}
		glyphs[width++] = utf8_narrow(glyph) ? glyph : '?';
	}

	while (width > 0 && glyphs[width - 1] == ' ') {
		width--;
	}

	return width;
}

static enum flavour
taste(const uint32_t *glyphs, int width)
{
	char text[FOOD_WIDTH + 1];
	size_t i;
	int x;

	for (x = 0; x < width; x++) {
		text[x] = glyphs[x] < 0x80 ? (char)tolower((int)glyphs[x]) : ' ';
	}
	text[width] = '\0';

	for (i = 0; i < sizeof spicy / sizeof spicy[0]; i++) {
		if (strstr(text, spicy[i]) != NULL) {
			return FLAVOUR_ERROR;
		}
	}
	if (strstr(text, "warn") != NULL) {
		return FLAVOUR_WARN;
	}
	return FLAVOUR_PLAIN;
}

static void
serve(struct food *f)
{
	static uint32_t glyphs[FOOD_WIDTH];
	enum flavour flavour;
	struct line *line;
	int width, left = 0, x;

	width = clean(f->pending, f->pending + f->pending_len, glyphs);
	f->pending_len = 0;

	for (x = 0; x < width; x++) {
		left += glyphs[x] != ' ';
	}
	if (left == 0) {
		return;
	}
	flavour = taste(glyphs, width);
	f->errors_seen += flavour == FLAVOUR_ERROR;
	f->fed = true;

	if (f->appetite < 1) {
		f->skipped++;
		return;
	}
	f->appetite -= 1;

	f->newest = (f->newest + 1) % FOOD_LINES;
	f->count += f->count < FOOD_LINES;

	line = &f->lines[f->newest];
	memcpy(line->glyphs, glyphs, (size_t)width * sizeof *glyphs);
	line->width = width;
	line->left = left;
	line->flavour = flavour;
}

static void
chew(struct food *f, const char *bytes, size_t len)
{
	size_t i;
	char ch;

	for (i = 0; i < len; i++) {
		ch = bytes[i];

		if (f->carriage) {
			f->carriage = false;
			if (ch != '\n') {
				f->pending_len = 0;
			}
		}
		if (ch == '\r') {
			f->carriage = true;
			continue;
		}
		if (ch == '\n') {
			serve(f);
			continue;
		}

		if (f->pending_len == sizeof f->pending) {
			serve(f);
		}
		f->pending[f->pending_len++] = ch;
	}
}

void
food_read(struct food *f, float dt)
{
	char chunk[CHUNK];
	long n, budget = FRAME_BUDGET;

	f->appetite = fminf(f->appetite + LINES_PER_SECOND * dt, LINES_AT_ONCE);

	while (f->open && budget > 0) {
		n = stream_read(chunk, sizeof chunk);
		if (n == 0) {
			return;
		}
		if (n == STREAM_ENDED) {
			if (f->pending_len > 0) {
				serve(f);
			}
			f->open = false;
			return;
		}

		if (f->passing) {
			f->passing = stream_pass(chunk, (size_t)n);
		}
		chew(f, chunk, (size_t)n);
		budget -= n;
	}
}

void
food_place(struct food *f, int width, int bottom)
{
	f->width = width;
	f->bottom = bottom;
}

static int
shown(const struct food *f)
{
	return f->count < f->bottom + 1 ? f->count : f->bottom + 1;
}

static int
slot(const struct food *f, int k)
{
	return (f->newest - k + FOOD_LINES) % FOOD_LINES;
}

static int
reach_of(const struct food *f, const struct line *line)
{
	return line->width < f->width ? line->width : f->width;
}

static vec3
spot(int x, int row)
{
	return v3((float)x + 0.5f, ((float)row + 0.5f) * UNITS_PER_ROW, 0);
}

bool
food_nearest(const struct food *f, vec3 from, vec3 *at)
{
	const struct line *line;
	float best = INFINITY, d;
	vec3 gap;
	int k, x, row;

	for (k = 0; k < shown(f); k++) {
		line = &f->lines[slot(f, k)];
		if (line->left == 0) {
			continue;
		}
		row = f->bottom - k;
		for (x = 0; x < reach_of(f, line); x++) {
			if (line->glyphs[x] == ' ') {
				continue;
			}
			gap = v3_sub(spot(x, row), from);
			d = v3_dot(gap, gap);
			if (line->flavour == FLAVOUR_ERROR) {
				d *= 0.36f;
			}
			if (d < best) {
				best = d;
				*at = spot(x, row);
			}
		}
	}

	return best < INFINITY;
}

void
food_eat(struct food *f, vec3 mouth, float reach, struct meal *meal)
{
	struct line *line;
	vec3 at;
	int k, x, row;

	memset(meal, 0, sizeof *meal);

	for (k = 0; k < shown(f); k++) {
		line = &f->lines[slot(f, k)];
		row = f->bottom - k;
		if (line->left == 0 || fabsf(spot(0, row).y - mouth.y) > reach) {
			continue;
		}

		for (x = 0; x < reach_of(f, line); x++) {
			at = spot(x, row);
			if (line->glyphs[x] == ' ' || v3_len(v3_sub(at, mouth)) > reach) {
				continue;
			}
			if (meal->count == MEAL_BITES) {
				return;
			}

			meal->bites[meal->count++] = (struct bite){
				.at = at,
				.glyph = line->glyphs[x],
				.flavour = line->flavour,
			};
			meal->errors += line->flavour == FLAVOUR_ERROR;
			meal->warnings += line->flavour == FLAVOUR_WARN;
			line->glyphs[x] = ' ';
			f->eaten++;

			if (--line->left == 0) {
				meal->lines++;
				f->swallowed++;
				f->errors += line->flavour == FLAVOUR_ERROR;
			}
		}
	}
}

void
food_draw(const struct food *f, struct canvas *c)
{
	const struct line *line;
	int k, x;

	for (k = 0; k < shown(f); k++) {
		line = &f->lines[slot(f, k)];
		for (x = 0; x < reach_of(f, line); x++) {
			if (line->glyphs[x] != ' ') {
				canvas_plot(c, x, f->bottom - k, FOOD_BACK,
				    line->glyphs[x], food_colour(line->flavour));
			}
		}
	}
}

vec3
food_colour(enum flavour flavour)
{
	switch (flavour) {
	case FLAVOUR_WARN:
		return v3(0.85f, 0.66f, 0.25f);
	case FLAVOUR_ERROR:
		return v3(0.92f, 0.32f, 0.30f);
	case FLAVOUR_PLAIN:
		break;
	}
	return v3(0.45f, 0.45f, 0.48f);
}
