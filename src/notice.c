#include <stdio.h>

#include "notice.h"
#include "words.h"

#define HOT_ENTER      0.85f
#define HOT_LEAVE      0.70f
#define CRAMPED_ENTER  0.85f
#define CRAMPED_LEAVE  0.80f
#define IDLE_CPU       0.15f
#define BUSY_CPU       0.60f
#define COMPILING_GAP  90.0f
#define BIG_TRAFFIC    5e6f
#define BIG_FOR        3.0f
#define SHRUNK         0.75f
#define GREW           1.33f
#define ERRORS_EVERY   10

static const long milestones[] = { 1000, 10000, 100000, 1000000 };
static const float together[] = { 600, 1800, 3600 };

#define LENGTH_OF(a) ((int)(sizeof(a) / sizeof((a)[0])))

static void
percent(char *out, size_t cap, float fraction)
{
	snprintf(out, cap, "%.0f%%", (double)(fraction * 100));
}

static void
alarms(struct mind *m, const struct senses *s, struct voice *v)
{
	const struct soil *soil = s->soil;
	char value[16];

	if (!m->hot && m->heat > HOT_ENTER) {
		m->hot = true;
		percent(value, sizeof value, soil->cpu);
		words_say(v, URGENCY_ALARM, SAY_HOT, value);
	} else if (m->hot && m->heat < HOT_LEAVE) {
		m->hot = false;
	}

	if (!m->cramped && soil->memory > CRAMPED_ENTER) {
		m->cramped = true;
		percent(value, sizeof value, soil->memory);
		words_say(v, URGENCY_ALARM, SAY_CRAMPED, value);
	} else if (m->cramped && soil->memory < CRAMPED_LEAVE) {
		m->cramped = false;
		words_say(v, URGENCY_NEWS, SAY_RELIEF, NULL);
	}
}

static void
traffic(float *lasted, float rate, enum occasion said, struct voice *v,
    float dt)
{
	char value[24];
	float before = *lasted;

	*lasted = rate > BIG_TRAFFIC ? *lasted + dt : 0;
	if (before < BIG_FOR && *lasted >= BIG_FOR) {
		words_rate(value, sizeof value, rate);
		words_say(v, URGENCY_NEWS, said, value);
	}
}

static void
machine(struct mind *m, const struct senses *s, struct voice *v, float dt)
{
	const struct soil *soil = s->soil;

	m->compiling_wait -= dt;
	if (m->heat < IDLE_CPU && soil->cpu > BUSY_CPU &&
	    m->compiling_wait <= 0) {
		m->compiling_wait = COMPILING_GAP;
		words_say(v, URGENCY_NEWS, SAY_COMPILING, NULL);
	}

	traffic(&m->big_in, soil->inflow, SAY_DOWNLOAD, v, dt);
	traffic(&m->big_out, soil->outflow, SAY_UPLOAD, v, dt);
}

static void
room(struct mind *m, const struct senses *s, struct voice *v)
{
	float area = s->width * s->height;

	if (m->area <= 0) {
		m->area = area;
		return;
	}
	if (area < m->area * SHRUNK) {
		words_say(v, URGENCY_NEWS, SAY_SHRUNK, NULL);
		m->area = area;
	} else if (area > m->area * GREW) {
		words_say(v, URGENCY_NEWS, SAY_GREW, NULL);
		m->area = area;
	}
}

static void
meals(struct mind *m, const struct food *f, struct voice *v)
{
	char value[24];
	long mark;

	if (m->milestones < LENGTH_OF(milestones) &&
	    f->eaten >= milestones[m->milestones]) {
		mark = milestones[m->milestones];
		if (mark >= 1000000) {
			snprintf(value, sizeof value, "%ldm", mark / 1000000);
		} else {
			snprintf(value, sizeof value, "%ldk", mark / 1000);
		}
		if (words_say(v, URGENCY_NEWS, SAY_MILESTONE, value)) {
			m->milestones++;
		}
	}

	if (f->errors_seen >= m->errors_noted + ERRORS_EVERY) {
		mark = f->errors_seen / ERRORS_EVERY * ERRORS_EVERY;
		snprintf(value, sizeof value, "%ld", mark);
		if (words_say(v, URGENCY_NEWS, SAY_ERROR_COUNT, value)) {
			m->errors_noted = mark;
		}
	}
}

static void
company(struct mind *m, struct voice *v, float dt)
{
	float mark;
	char value[16];

	m->together += dt;
	if (m->together_marks >= LENGTH_OF(together)) {
		return;
	}
	mark = together[m->together_marks];
	if (m->together < mark) {
		return;
	}

	if (mark >= 3600) {
		m->together_marks += words_say(v, URGENCY_IDLE, SAY_HOUR, NULL);
	} else {
		snprintf(value, sizeof value, "%.0f", (double)(mark / 60));
		m->together_marks += words_say(v, URGENCY_IDLE, SAY_TOGETHER,
		    value);
	}
}

void
notice(struct mind *m, const struct senses *s, struct voice *v, float dt)
{
	alarms(m, s, v);
	machine(m, s, v, dt);
	room(m, s, v);
	meals(m, s->food, v);
	company(m, v, dt);
}
