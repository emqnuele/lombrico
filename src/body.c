#include <string.h>

#include "body.h"
#include "rng.h"

#define BODY_SHORTEST 16
#define WAVE_WIDTH    0.06f
#define SWELL_MOST    0.6f

void
body_init(struct body *b, vec3 head, int length, float girth)
{
	int i;

	memset(b, 0, sizeof *b);
	b->length = length;
	b->growth = (float)length;
	b->girth = girth;

	for (i = 0; i < length; i++) {
		b->spine[i] = v3(head.x - (float)i * BODY_SPACING, head.y, 0);
	}
}

vec3
body_head(const struct body *b)
{
	return b->spine[0];
}

static void
grow(struct body *b, float length, float dt)
{
	vec3 tail, back;
	int want;

	length = clampf(length, BODY_SHORTEST, BODY_SEGMENTS);
	b->growth = followf(b->growth, length, 0.5f, dt);
	want = (int)lrintf(b->growth);

	while (b->length < want) {
		tail = b->spine[b->length - 1];
		back = v3_sub(tail, b->spine[b->length - 2]);
		back = v3_len(back) > 1e-4f ? v3_norm(back) : v3(-1, 0, 0);
		b->spine[b->length++] = v3_add(tail, v3_scale(back, BODY_SPACING));
	}
	if (b->length > want) {
		b->length = want;
	}
}

static void
steer(struct body *b, const struct gait *g, vec3 target, float dt)
{
	vec3 head = b->spine[0];
	float want = atan2f(target.y - head.y, target.x - head.x);
	float limit = g->turn * dt;
	float heading;

	b->angle += clampf(wrap_angle(want - b->angle), -limit, limit);
	b->angle += rng_range(-1, 1) * g->jitter * dt;
	b->angle = wrap_angle(b->angle);
	b->swing = fmodf(b->swing + TAU * g->tempo * dt, TAU);

	heading = b->angle + sinf(b->swing) * g->sway;
	b->spine[0] = v3_add(head,
	    v3_scale(v3(cosf(heading), sinf(heading), 0), g->speed * dt));
}

static void
follow(struct body *b)
{
	vec3 gap;
	float len;
	int i;

	for (i = 1; i < b->length; i++) {
		gap = v3_sub(b->spine[i], b->spine[i - 1]);
		len = v3_len(gap);
		if (len > 1e-4f) {
			b->spine[i] = v3_add(b->spine[i - 1],
			    v3_scale(gap, BODY_SPACING / len));
		}
	}
}

void
body_crawl(struct body *b, const struct gait *g, vec3 target, float dt)
{
	grow(b, g->length, dt);
	steer(b, g, target, dt);
	follow(b);
}

void
body_flow(struct body *b, float dt)
{
	struct wave *w;
	int i;

	for (i = 0; i < BODY_WAVES; i++) {
		w = &b->waves[i];
		if (w->size <= 0) {
			continue;
		}
		w->s += w->speed * dt;
		if (w->s < -0.2f || w->s > 1.2f) {
			w->size = 0;
		}
	}
}

void
body_wave(struct body *b, struct wave w)
{
	struct wave *slot = &b->waves[0];
	int i;

	for (i = 1; i < BODY_WAVES; i++) {
		if (b->waves[i].size < slot->size) {
			slot = &b->waves[i];
		}
	}
	*slot = w;
}

float
body_swelling(const struct body *b, float s, vec3 *tint, float *stain)
{
	const struct wave *w;
	vec3 ink = v3(0, 0, 0);
	float swell = 0, weight = 0, d, k;
	int i;

	for (i = 0; i < BODY_WAVES; i++) {
		w = &b->waves[i];
		if (w->size <= 0) {
			continue;
		}
		d = (s - w->s) / WAVE_WIDTH;
		k = expf(-d * d);
		swell += w->size * k;
		ink = v3_add(ink, v3_scale(w->tint, w->stain * k));
		weight += w->stain * k;
	}

	*tint = weight > 1e-4f ? v3_scale(ink, 1.0f / weight) : ink;
	*stain = clampf(weight, 0, 1);

	return SWELL_MOST * (1 - expf(-swell / SWELL_MOST));
}
