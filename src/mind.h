#ifndef LOMBRICO_MIND_H
#define LOMBRICO_MIND_H

#include <stdbool.h>

#include "body.h"
#include "ear.h"
#include "food.h"
#include "leaves.h"
#include "motes.h"
#include "press.h"
#include "skin.h"
#include "soil.h"
#include "voice.h"

enum mood {
	MOOD_WANDER,
	MOOD_EAT,
	MOOD_DANCE,
	MOOD_SLEEP,
};

enum whim {
	WHIM_NONE,
	WHIM_TAIL,
	WHIM_ZOOMIES,
	WHIM_STARE,
	WHIM_STRETCH,
	WHIMS,
};

struct senses {
	const struct soil    *soil;
	const struct hearing *hearing;
	struct food          *food;
	struct leaves        *leaves;
	float                 width;
	float                 height;
	int                   hour;
	bool                  touched;
};

struct mind {
	enum mood    mood;
	struct gait  gait;
	struct look  look;
	vec3         target;

	float        idle;
	float        heat;
	float        spice;
	float        restless;
	float        chatter;
	float        nap;
	float        snore;
	float        step;
	float        encore;

	enum whim    whim;
	float        whim_left;
	float        bored;

	float        joy;
	float        flee;
	vec3         flee_from;
	float        blink;
	float        blink_wait;
	float        grumble;

	float        bolt;
	vec3         bolt_to;
	float        startle_wait;
	float        since_beat;

	int          digesting;
	enum flavour richest;
	float        gulp_wait;
	float        fullness;
	bool         stuffed;
	long         skipped;
	float        overfed_wait;

	float        inflow;
	float        outflow;
	float        ripple_wait;

	bool         hot;
	bool         cramped;
	bool         tasted;
	bool         judged;
	float        compiling_wait;
	float        big_in;
	float        big_out;
	float        area;
	float        together;
	int          together_marks;
	int          milestones;
	long         errors_noted;
};

void mind_init(struct mind *);
void mind_greet(struct voice *, int hour);
void mind_press(struct mind *, const struct press *, const struct senses *,
    const struct body *, struct voice *, struct motes *);
void mind_think(struct mind *, const struct senses *, struct body *,
    struct voice *, struct motes *, float dt);

#endif
