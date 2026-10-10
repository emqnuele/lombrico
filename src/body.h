#ifndef LOMBRICO_BODY_H
#define LOMBRICO_BODY_H

#include "vec.h"

#define BODY_SEGMENTS 192
#define BODY_WAVES    32
#define BODY_SPACING  1.0f

struct wave {
	float s;
	float speed;
	float size;
	vec3  tint;
	float stain;
};

struct gait {
	float speed;
	float turn;
	float sway;
	float tempo;
	float jitter;
	float length;
};

struct body {
	vec3        spine[BODY_SEGMENTS];
	int         length;
	float       growth;
	float       angle;
	float       swing;
	float       girth;
	struct wave waves[BODY_WAVES];
};

void  body_init(struct body *, vec3 head, int length, float girth);
void  body_crawl(struct body *, const struct gait *, vec3 target, float dt);
void  body_flow(struct body *, float dt);
void  body_wave(struct body *, struct wave);
float body_swelling(const struct body *, float s, vec3 *tint, float *stain);
vec3  body_head(const struct body *);

#endif
