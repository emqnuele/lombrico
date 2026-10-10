#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "utf8.h"
#include "voice.h"

#define LETTERS_PER_SECOND 28.0f
#define LINGER             2.6f
#define FADE               0.6f
#define SETTLE             0.8f
#define QUIET_GAP          6.0f

#define RULE        0x2500u
#define SIDE        0x2502u
#define TOP_LEFT    0x256du
#define TOP_RIGHT   0x256eu
#define BOTTOM_LEFT 0x2570u
#define BOTTOM_RIGHT 0x256fu
#define TAIL_DOWN   0x252cu
#define TAIL_UP     0x2534u

static const vec3 FRAME = { 0.45f, 0.45f, 0.50f };
static const vec3 WORDS = { 0.95f, 0.94f, 0.90f };

static int
glyphs_in(const char *text)
{
	int count = 0;

	for (; *text != '\0'; text++) {
		count += ((unsigned char)*text & 0xc0) != 0x80;
	}
	return count;
}

static void
trim_partial(char *text, size_t len)
{
	size_t start = len;

	while (start > 0 && ((unsigned char)text[start - 1] & 0xc0) == 0x80) {
		start--;
	}
	if (start > 0) {
		const char *p = text + start - 1;
		if (utf8_decode(&p, text + len) == UTF8_INVALID) {
			text[start - 1] = '\0';
		}
	}
}

bool
voice_say(struct voice *v, enum urgency urgency, const char *format, ...)
{
	bool speaking = v->age < v->life;
	bool settled = v->age > (float)v->length / LETTERS_PER_SECOND + SETTLE;
	bool interrupts;
	va_list args;
	int written;

	interrupts = urgency > v->urgency ||
	    (urgency == URGENCY_REPLY && v->urgency == URGENCY_REPLY);
	if (speaking && !settled && !interrupts) {
		return false;
	}

	if (urgency == URGENCY_IDLE && (speaking || v->quiet < QUIET_GAP)) {
		return false;
	}

	va_start(args, format);
	written = vsnprintf(v->line, sizeof v->line, format, args);
	va_end(args);

	if (written < 0) {
		v->line[0] = '\0';
	} else if ((size_t)written >= sizeof v->line) {
		trim_partial(v->line, sizeof v->line - 1);
	}

	v->length = glyphs_in(v->line);
	v->urgency = urgency;
	v->age = 0;
	v->life = (float)v->length / LETTERS_PER_SECOND + LINGER;
	v->quiet = 0;
	return true;
}

void
voice_age(struct voice *v, float dt)
{
	if (v->age < v->life) {
		v->age += dt;
	} else {
		v->quiet += dt;
	}
}

static void
rule(struct canvas *c, int left, int y, int width, uint32_t open,
    uint32_t close, int tail_x, uint32_t tail, vec3 ink)
{
	uint32_t glyph;
	int x;

	for (x = 0; x < width; x++) {
		if (x == 0) {
			glyph = open;
		} else if (x == width - 1) {
			glyph = close;
		} else if (left + x == tail_x) {
			glyph = tail;
		} else {
			glyph = RULE;
		}
		canvas_plot(c, left + x, y, CANVAS_FRONT, glyph, ink);
	}
}

void
voice_draw(const struct voice *v, struct canvas *c, vec3 head, float girth)
{
	const char *text = v->line, *end = v->line + strlen(v->line);
	int width = v->length + 4;
	int hx = (int)floorf(head.x);
	int hy = (int)floorf(head.y / UNITS_PER_ROW);
	int clearance = (int)ceilf(girth / UNITS_PER_ROW) + 1;
	int shown, top, left, tail_x, x;
	bool above;
	float fade;

	if (v->age >= v->life || width > c->width) {
		return;
	}

	fade = 1 - smoothstepf(v->life - FADE, v->life, v->age);
	shown = (int)(v->age * LETTERS_PER_SECOND);

	top = hy - clearance - 3;
	above = top >= 0;
	if (!above) {
		top = hy + clearance + 1;
	}
	left = (int)clampf((float)(hx - width / 2), 0, (float)(c->width - width));
	tail_x = (int)clampf((float)hx, (float)left + 1, (float)(left + width - 2));

	rule(c, left, top, width, TOP_LEFT, TOP_RIGHT,
	    tail_x, above ? RULE : TAIL_UP, v3_scale(FRAME, fade));
	rule(c, left, top + 2, width, BOTTOM_LEFT, BOTTOM_RIGHT,
	    tail_x, above ? TAIL_DOWN : RULE, v3_scale(FRAME, fade));

	canvas_plot(c, left, top + 1, CANVAS_FRONT, SIDE, v3_scale(FRAME, fade));
	canvas_plot(c, left + width - 1, top + 1, CANVAS_FRONT, SIDE,
	    v3_scale(FRAME, fade));

	for (x = 1; x < width - 1; x++) {
		uint32_t glyph = ' ';

		if (x >= 2 && x - 2 < shown && text < end) {
			glyph = utf8_decode(&text, end);
		}
		canvas_plot(c, left + x, top + 1, CANVAS_FRONT, glyph,
		    v3_scale(WORDS, fade));
	}
}
