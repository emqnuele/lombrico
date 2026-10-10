#ifndef LOMBRICO_MOTES_H
#define LOMBRICO_MOTES_H

#include <stdint.h>

#include "canvas.h"

#define MOTES 256

struct mote {
	vec3     at;
	vec3     velocity;
	vec3     colour;
	float    gravity;
	float    age;
	float    life;
	uint32_t glyph;
};

struct motes {
	struct mote pool[MOTES];
	int         next;
};

void motes_emit(struct motes *, struct mote);
void motes_drift(struct motes *, float dt);
void motes_draw(const struct motes *, struct canvas *);

#endif
