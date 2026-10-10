#include "motes.h"

void
motes_emit(struct motes *m, struct mote mote)
{
	mote.age = 0;
	m->pool[m->next] = mote;
	m->next = (m->next + 1) % MOTES;
}

void
motes_drift(struct motes *m, float dt)
{
	struct mote *p;
	int i;

	for (i = 0; i < MOTES; i++) {
		p = &m->pool[i];
		if (p->age >= p->life) {
			continue;
		}
		p->age += dt;
		p->velocity.y += p->gravity * dt;
		p->at = v3_add(p->at, v3_scale(p->velocity, dt));
	}
}

void
motes_draw(const struct motes *m, struct canvas *c)
{
	const struct mote *p;
	float fade;
	int i;

	for (i = 0; i < MOTES; i++) {
		p = &m->pool[i];
		if (p->age >= p->life) {
			continue;
		}
		fade = 1 - smoothstepf(0.5f, 1, p->age / p->life);
		canvas_point(c, p->at, CANVAS_FRONT / 2, p->glyph,
		    v3_scale(p->colour, fade));
	}
}
