#ifndef LOMBRICO_FOOD_H
#define LOMBRICO_FOOD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canvas.h"

#define FOOD_LINES 256
#define FOOD_WIDTH 512
#define MEAL_BITES 96

enum flavour {
	FLAVOUR_PLAIN,
	FLAVOUR_WARN,
	FLAVOUR_ERROR,
};

struct line {
	uint32_t     glyphs[FOOD_WIDTH];
	int          width;
	int          left;
	enum flavour flavour;
};

struct bite {
	vec3         at;
	uint32_t     glyph;
	enum flavour flavour;
};

struct meal {
	struct bite bites[MEAL_BITES];
	int         count;
	int         lines;
	int         errors;
	int         warnings;
};

struct food {
	bool        open;
	bool        passing;
	bool        fed;
	float       appetite;

	struct line lines[FOOD_LINES];
	int         newest;
	int         count;

	char        pending[FOOD_WIDTH * 4];
	size_t      pending_len;
	bool        carriage;

	int         width;
	int         bottom;

	long        eaten;
	long        swallowed;
	long        errors;
	long        errors_seen;
	long        skipped;
};

void food_open(struct food *);
void food_read(struct food *, float dt);
void food_place(struct food *, int width, int bottom);
bool food_nearest(const struct food *, vec3 from, vec3 *at);
void food_eat(struct food *, vec3 mouth, float reach, struct meal *);
void food_draw(const struct food *, struct canvas *);
vec3 food_colour(enum flavour);

#endif
