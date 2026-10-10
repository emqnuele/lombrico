#include <stdlib.h>
#include <string.h>

#include "canvas.h"
#include "utf8.h"

int
canvas_resize(struct canvas *c, int width, int height)
{
	size_t area = (size_t)width * (size_t)height;
	struct cell *cells;
	float *depth;

	if (width == c->width && height == c->height) {
		return 0;
	}

	cells = realloc(c->cells, area * sizeof *cells);
	if (cells == NULL) {
		return -1;
	}
	c->cells = cells;

	depth = realloc(c->depth, area * sizeof *depth);
	if (depth == NULL) {
		return -1;
	}
	c->depth = depth;

	c->width = width;
	c->height = height;
	canvas_clear(c);

	return 0;
}

void
canvas_free(struct canvas *c)
{
	free(c->cells);
	free(c->depth);
	memset(c, 0, sizeof *c);
}

void
canvas_clear(struct canvas *c)
{
	size_t area = (size_t)c->width * (size_t)c->height;
	size_t i;

	for (i = 0; i < area; i++) {
		c->cells[i] = (struct cell){ .glyph = ' ' };
		c->depth[i] = -INFINITY;
	}
}

static uint8_t
channel(float v)
{
	return (uint8_t)lrintf(clampf(v, 0.0f, 1.0f) * 255.0f);
}

void
canvas_plot(struct canvas *c, int x, int y, float z, uint32_t glyph,
    vec3 colour)
{
	size_t at;

	if (x < 0 || y < 0 || x >= c->width || y >= c->height) {
		return;
	}

	at = (size_t)y * (size_t)c->width + (size_t)x;
	if (z <= c->depth[at]) {
		return;
	}

	c->depth[at] = z;
	c->cells[at] = (struct cell){
		.glyph = utf8_narrow(glyph) ? glyph : '?',
		.r = channel(colour.x),
		.g = channel(colour.y),
		.b = channel(colour.z),
	};
}

void
canvas_point(struct canvas *c, vec3 at, float z, uint32_t glyph, vec3 colour)
{
	canvas_plot(c, (int)floorf(at.x), (int)floorf(at.y / UNITS_PER_ROW), z,
	    glyph, colour);
}

int
canvas_print(struct canvas *c, int x, int y, float z, vec3 colour,
    const char *text)
{
	const char *end = text + strlen(text);
	int start = x;

	while (text < end) {
		canvas_plot(c, x++, y, z, utf8_decode(&text, end), colour);
	}

	return x - start;
}
