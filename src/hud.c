#include <stdio.h>

#include "hud.h"
#include "words.h"

static const vec3 QUIET = { 0.42f, 0.42f, 0.46f };
static const vec3 LOUD  = { 0.80f, 0.80f, 0.84f };

static int
field(struct canvas *c, int x, int y, const char *name, const char *value)
{
	x += canvas_print(c, x, y, CANVAS_FRONT, QUIET, name);
	x += canvas_print(c, x, y, CANVAS_FRONT, QUIET, " ");
	x += canvas_print(c, x, y, CANVAS_FRONT, LOUD, value);
	return x + 3;
}

void
hud_draw(struct canvas *c, const struct soil *soil,
    const struct hearing *hearing, const struct food *food)
{
	char value[32];
	int x = 1, y = c->height - 1;

	snprintf(value, sizeof value, "%.0f%%", (double)(soil->cpu * 100));
	x = field(c, x, y, "cpu", value);

	snprintf(value, sizeof value, "%.0f%%", (double)(soil->memory * 100));
	x = field(c, x, y, "mem", value);

	if (soil->load >= 0) {
		snprintf(value, sizeof value, "%.2f", (double)soil->load);
		x = field(c, x, y, "load", value);
	}

	words_rate(value, sizeof value, soil->inflow);
	x = field(c, x, y, "\xe2\x86\x93", value);

	words_rate(value, sizeof value, soil->outflow);
	x = field(c, x, y, "\xe2\x86\x91", value);

	snprintf(value, sizeof value, "%.0f%%", (double)(hearing->level * 100));
	x = field(c, x, y, "\xe2\x99\xaa", value);

	if (food->fed) {
		snprintf(value, sizeof value, "%ld chars, %ld lines, %ld errors",
		    food->eaten, food->swallowed, food->errors);
		x = field(c, x, y, "ate", value);
	}
	if (food->skipped > 0) {
		snprintf(value, sizeof value, "%ld lines", food->skipped);
		field(c, x, y, "skipped", value);
	}
}
