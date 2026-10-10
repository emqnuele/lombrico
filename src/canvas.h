#ifndef LOMBRICO_CANVAS_H
#define LOMBRICO_CANVAS_H

#include <stdint.h>

#include "vec.h"

#define CANVAS_FRONT 1e9f

#define UNITS_PER_ROW 2.0f

struct cell {
	uint32_t glyph;
	uint8_t  r, g, b;
};

struct canvas {
	int          width;
	int          height;
	struct cell *cells;
	float       *depth;
};

int  canvas_resize(struct canvas *, int width, int height);
void canvas_free(struct canvas *);
void canvas_clear(struct canvas *);
void canvas_plot(struct canvas *, int x, int y, float z, uint32_t glyph,
    vec3 colour);
void canvas_point(struct canvas *, vec3 at, float z, uint32_t glyph,
    vec3 colour);
int  canvas_print(struct canvas *, int x, int y, float z, vec3 colour,
    const char *text);

#endif
