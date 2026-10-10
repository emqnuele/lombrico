#ifndef LOMBRICO_SKIN_H
#define LOMBRICO_SKIN_H

#include <stdbool.h>

#include "body.h"
#include "canvas.h"

enum eyes {
	EYES_OPEN,
	EYES_SHUT,
	EYES_WIDE,
};

struct look {
	vec3      colour;
	float     glow;
	float     swell;
	float     undulate;
	float     roll;
	float     phase;
	enum eyes eyes;
};

void skin_draw(struct canvas *, const struct body *, const struct look *);

#endif
