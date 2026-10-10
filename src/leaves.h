#ifndef LOMBRICO_LEAVES_H
#define LOMBRICO_LEAVES_H

#include <stdbool.h>

#include "canvas.h"

#define LEAVES 8

struct leaf {
	vec3  at;
	float age;
	bool  present;
};

struct leaves {
	struct leaf pool[LEAVES];
	int         next;
};

void leaves_drop(struct leaves *, vec3 at);
void leaves_age(struct leaves *, float dt);
bool leaves_nearest(const struct leaves *, vec3 from, vec3 *at);
int  leaves_eat(struct leaves *, vec3 mouth, float reach);
void leaves_draw(const struct leaves *, struct canvas *);

#endif
