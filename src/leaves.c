#include "leaves.h"

#define FALL     6.0f
#define FALLING  0.4f
#define BACK     (-CANVAS_FRONT / 2)
#define LEAF     0x2663u

static const vec3 GREEN = { 0.35f, 0.85f, 0.40f };

void
leaves_drop(struct leaves *l, vec3 at)
{
	l->pool[l->next] = (struct leaf){ .at = at, .present = true };
	l->next = (l->next + 1) % LEAVES;
}

void
leaves_age(struct leaves *l, float dt)
{
	int i;

	for (i = 0; i < LEAVES; i++) {
		l->pool[i].age += dt;
	}
}

static bool
landed(const struct leaf *leaf)
{
	return leaf->present && leaf->age >= FALLING;
}

bool
leaves_nearest(const struct leaves *l, vec3 from, vec3 *at)
{
	float best = INFINITY, d;
	vec3 gap;
	int i;

	for (i = 0; i < LEAVES; i++) {
		if (!landed(&l->pool[i])) {
			continue;
		}
		gap = v3_sub(l->pool[i].at, from);
		d = v3_dot(gap, gap);
		if (d < best) {
			best = d;
			*at = l->pool[i].at;
		}
	}
	return best < INFINITY;
}

int
leaves_eat(struct leaves *l, vec3 mouth, float reach)
{
	int i, eaten = 0;

	for (i = 0; i < LEAVES; i++) {
		if (landed(&l->pool[i]) &&
		    v3_len(v3_sub(l->pool[i].at, mouth)) <= reach) {
			l->pool[i].present = false;
			eaten++;
		}
	}
	return eaten;
}

void
leaves_draw(const struct leaves *l, struct canvas *c)
{
	const struct leaf *leaf;
	vec3 at;
	float glint;
	int i;

	for (i = 0; i < LEAVES; i++) {
		leaf = &l->pool[i];
		if (!leaf->present) {
			continue;
		}
		at = leaf->at;
		at.y -= FALL * (1 - smoothstepf(0, FALLING, leaf->age));
		glint = 0.85f + 0.15f * sinf(leaf->age * 3);
		canvas_point(c, at, BACK, LEAF, v3_scale(GREEN, glint));
	}
}
