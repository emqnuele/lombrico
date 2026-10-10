#include <stdio.h>
#include <string.h>

#include "mind.h"
#include "notice.h"
#include "rng.h"
#include "words.h"

#define SLEEP_AFTER    30.0f
#define SLEEP_AT_NIGHT 15.0f
#define SETTLING       8.0f
#define DANCE_ENTER    0.6f
#define DANCE_LEAVE    0.35f
#define MUSIC_HEARD    0.4f
#define SLOW_SONG      95.0f
#define FAST_SONG      140.0f

#define BORED_AFTER    12.0f
#define WHIM_CHANCE    0.08f

#define MOUTHFUL       12
#define GULP_GAP       0.45f
#define OVERFED_GAP    30.0f
#define FULL           400.0f
#define BURSTING       600.0f
#define DIGESTION      20.0f
#define LEAF_MEAL      40.0f

#define TOO_MANY_PETS  4.0f
#define FLEE_TIME      2.5f
#define BOLT_TIME      0.8f
#define STARTLE_GAP    6.0f
#define GRUMBLE_GAP    8.0f

#define RIPPLE_BYTES   262144.0f
#define RIPPLE_GAP     0.2f

#define NOTE           0x266au
#define NOTES          0x266bu
#define HEART          0x2665u
#define DEGREE         0xb0u

static const vec3 CALM  = { 0.93f, 0.18f, 0.23f };
static const vec3 HOT   = { 1.00f, 0.55f, 0.12f };
static const vec3 PARTY = { 1.00f, 0.22f, 0.62f };
static const vec3 SNORE = { 0.55f, 0.55f, 0.65f };
static const vec3 LEAFY = { 0.45f, 0.95f, 0.45f };
static const vec3 PINK  = { 1.00f, 0.45f, 0.70f };
static const vec3 WHITE = { 1.00f, 1.00f, 0.95f };

void
mind_init(struct mind *m)
{
	memset(m, 0, sizeof *m);
	m->mood = MOOD_WANDER;
	m->chatter = rng_range(8, 14);
	m->encore = rng_range(20, 30);
	m->blink_wait = rng_range(2.5f, 6);
	m->look.colour = CALM;
}

static bool
night(int hour)
{
	return hour >= 23 || hour < 6;
}

void
mind_greet(struct voice *v, int hour)
{
	enum occasion o = SAY_NIGHT;

	if (hour >= 5 && hour < 12) {
		o = SAY_MORNING;
	} else if (hour >= 12 && hour < 18) {
		o = SAY_AFTERNOON;
	} else if (hour >= 18 && hour < 23) {
		o = SAY_EVENING;
	}
	words_say(v, URGENCY_NEWS, o, NULL);
}

static float
settled(const struct mind *m)
{
	return m->mood == MOOD_SLEEP ? smoothstepf(0, SETTLING, m->nap) : 0;
}

static vec3
inside(const struct senses *s, vec3 at, float margin)
{
	margin = fminf(margin, fminf(s->width, s->height) / 2);
	at.x = clampf(at.x, margin, s->width - margin);
	at.y = clampf(at.y, margin, s->height - margin);
	return at;
}

static vec3
somewhere(const struct senses *s, float girth)
{
	float margin = girth * 2 + 2;

	return inside(s, v3(rng_range(0, s->width), rng_range(0, s->height), 0),
	    margin);
}

static void
feel(struct mind *m, const struct senses *s, bool hungry, float dt)
{
	const struct hearing *h = s->hearing;
	bool stirred;

	m->heat = followf(m->heat, s->soil->cpu, 1.5f, dt);
	m->spice *= expf(-0.4f * dt);

	stirred = s->touched || hungry || h->jolted || s->soil->cpu > 0.25f ||
	    h->music > 0.2f || m->spice > 0.1f;
	m->idle = stirred ? 0 : m->idle + dt;
}

static void
verdict(struct mind *m, const struct food *f, struct voice *v)
{
	char count[24];

	if (m->judged || f->open || !f->fed) {
		return;
	}
	m->judged = true;

	if (f->errors_seen == 0) {
		words_say(v, URGENCY_NEWS, SAY_CLEAN, NULL);
	} else {
		snprintf(count, sizeof count, "%ld", f->errors_seen);
		words_say(v, URGENCY_NEWS, SAY_DIRTY, count);
	}
}

static void
announce(struct mind *m, const struct senses *s, struct voice *v,
    enum mood from)
{
	float bpm = s->hearing->tempo;
	char tempo[16];

	m->whim = WHIM_NONE;
	snprintf(tempo, sizeof tempo, "%.0f", (double)bpm);

	switch (m->mood) {
	case MOOD_EAT:
		words_say(v, URGENCY_NEWS, m->tasted ? SAY_SNACK : SAY_FIRST_FOOD,
		    NULL);
		m->tasted = true;
		break;
	case MOOD_DANCE:
		if (bpm > 0 && bpm < SLOW_SONG) {
			words_say(v, URGENCY_NEWS, SAY_SLOW_TUNE, tempo);
		} else if (bpm > FAST_SONG) {
			words_say(v, URGENCY_NEWS, SAY_FAST_TUNE, tempo);
		} else {
			words_say(v, URGENCY_NEWS, SAY_TUNE, NULL);
		}
		break;
	case MOOD_SLEEP:
		m->nap = 0;
		words_say(v, URGENCY_IDLE, SAY_NAP, NULL);
		break;
	case MOOD_WANDER:
		if (from == MOOD_SLEEP) {
			words_say(v, URGENCY_NEWS, SAY_WAKE, NULL);
		} else if (from == MOOD_DANCE) {
			words_say(v, URGENCY_IDLE, SAY_SONG_OVER, NULL);
		} else if (from == MOOD_EAT) {
			verdict(m, s->food, v);
		}
		break;
	}
}

static void
choose(struct mind *m, const struct senses *s, struct voice *v, bool hungry)
{
	enum mood was = m->mood;
	float music = s->hearing->music;
	float drowsy = night(s->hour) ? SLEEP_AT_NIGHT : SLEEP_AFTER;

	if (hungry) {
		m->mood = MOOD_EAT;
	} else if (music > (was == MOOD_DANCE ? DANCE_LEAVE : DANCE_ENTER)) {
		m->mood = MOOD_DANCE;
	} else if (m->idle > drowsy) {
		m->mood = MOOD_SLEEP;
	} else {
		m->mood = MOOD_WANDER;
	}

	if (m->mood != was) {
		announce(m, s, v, was);
	}
}

static void
fancy(struct mind *m, const struct senses *s, const struct body *b,
    struct voice *v)
{
	static const struct {
		enum occasion said;
		float         lasts;
	} whims[WHIMS] = {
		[WHIM_TAIL]    = { SAY_TAIL, 6 },
		[WHIM_ZOOMIES] = { SAY_ZOOMIES, 3.5f },
		[WHIM_STARE]   = { SAY_STARE, 3 },
		[WHIM_STRETCH] = { SAY_STRETCH, 4 },
	};
	vec3 head = body_head(b);

	m->whim = (enum whim)(WHIM_TAIL + rng_next() % (WHIMS - WHIM_TAIL));
	m->whim_left = whims[m->whim].lasts;
	m->bored = 0;
	words_say(v, URGENCY_NEWS, whims[m->whim].said, NULL);

	if (m->whim == WHIM_ZOOMIES) {
		m->target = inside(s, v3(head.x < s->width / 2 ? s->width : 0,
		    rng_range(0, s->height), 0), b->girth * 2);
	}
}

static void
indulge(struct mind *m, const struct senses *s, const struct body *b,
    float dt)
{
	vec3 head = body_head(b);

	m->whim_left -= dt;
	if (m->whim_left <= 0) {
		m->whim = WHIM_NONE;
		return;
	}

	switch (m->whim) {
	case WHIM_TAIL:
		m->target = b->spine[b->length - 1];
		m->gait = (struct gait){
			.speed = 12, .turn = 3.5f, .sway = 0.1f, .tempo = 1,
		};
		break;
	case WHIM_ZOOMIES:
		if (v3_len(v3_sub(m->target, head)) < b->girth * 3) {
			m->target = inside(s, v3(head.x < s->width / 2 ? s->width : 0,
			    rng_range(0, s->height), 0), b->girth * 2);
		}
		m->gait = (struct gait){
			.speed = 32, .turn = 4, .sway = 0.2f, .tempo = 2.5f,
		};
		break;
	case WHIM_STARE:
		m->target = v3_add(head, v3(cosf(b->angle), sinf(b->angle), 0));
		m->gait = (struct gait){ .turn = 1 };
		break;
	case WHIM_STRETCH:
		m->gait = (struct gait){
			.speed = 5, .turn = 1.5f, .sway = 0.2f, .tempo = 0.5f,
		};
		break;
	case WHIM_NONE:
	case WHIMS:
		break;
	}
}

static bool
vitals(const struct soil *soil, struct voice *v)
{
	if (soil->load < 0) {
		return voice_say(v, URGENCY_IDLE, "cpu %.0f%% · mem %.0f%%",
		    (double)(soil->cpu * 100), (double)(soil->memory * 100));
	}
	return voice_say(v, URGENCY_IDLE, "cpu %.0f%% · mem %.0f%% · load %.1f",
	    (double)(soil->cpu * 100), (double)(soil->memory * 100),
	    (double)soil->load);
}

static void
wander(struct mind *m, const struct senses *s, const struct body *b,
    struct voice *v, float dt)
{
	if (m->whim != WHIM_NONE) {
		indulge(m, s, b, dt);
		if (m->whim != WHIM_NONE) {
			return;
		}
	}

	m->bored += dt;
	if (m->bored > BORED_AFTER && rng_unit() < WHIM_CHANCE * dt) {
		fancy(m, s, b, v);
		indulge(m, s, b, 0);
		return;
	}

	m->restless -= dt;
	if (m->restless <= 0 ||
	    v3_len(v3_sub(m->target, body_head(b))) < b->girth * 2) {
		m->target = somewhere(s, b->girth);
		m->restless = rng_range(6, 10);
	}

	m->chatter -= dt;
	if (m->chatter <= 0 && (rng_next() % 2 ? vitals(s->soil, v) :
	    words_say(v, URGENCY_IDLE, SAY_THOUGHT, NULL))) {
		m->chatter = rng_range(30, 50);
	}

	m->gait = (struct gait){
		.speed = 9, .turn = 2.2f, .sway = 0.45f, .tempo = 0.9f,
	};
}

static void
eat(struct mind *m, const struct senses *s, const struct body *b, vec3 meal,
    float dt)
{
	m->target = v3_follow(m->target, inside(s, meal, b->girth * 0.5f), 8, dt);
	m->gait = (struct gait){
		.speed = 14, .turn = 5, .sway = 0.15f, .tempo = 1.6f,
	};
}

static void
dance(struct mind *m, const struct senses *s, const struct body *b,
    struct voice *v, float dt)
{
	const struct hearing *h = s->hearing;
	float pace = h->tempo > 0 ? clampf(h->tempo / 120, 0.5f, 1.8f) : 1;
	vec3 head = body_head(b);
	char tempo[16];

	if (v3_len(v3_sub(m->target, head)) < b->girth * 3) {
		m->step += 1.4f * pace * dt;
	}
	m->target = v3(s->width / 2 + s->width * 0.3f * sinf(m->step),
	    s->height / 2 + s->height * 0.25f * sinf(m->step) * cosf(m->step) * 2,
	    0);

	m->gait = (struct gait){
		.speed = (11 + 8 * h->beat) * pace, .turn = 3.5f * pace,
		.sway = 0.3f, .tempo = 1.2f * pace,
	};

	m->encore -= dt;
	if (m->encore <= 0 && h->tempo > 0) {
		snprintf(tempo, sizeof tempo, "%.0f", (double)h->tempo);
		if (words_say(v, URGENCY_IDLE, SAY_TEMPO, tempo)) {
			m->encore = rng_range(40, 60);
		}
	}
}

static void
doze(struct mind *m, const struct body *b, struct motes *motes, float dt)
{
	float rest = settled(m);
	float speed = 6 * (1 - rest);
	float radius = lerpf(b->girth * 6, b->girth * 2.4f, rest);
	float aside = b->angle + 1.2f;

	m->nap += dt;
	m->target = v3_add(body_head(b), v3(cosf(aside) * 10, sinf(aside) * 10, 0));
	m->gait = (struct gait){ .speed = speed, .turn = speed / radius };

	m->snore -= dt;
	if (rest > 0.9f && m->snore <= 0) {
		m->snore = 2.2f;
		motes_emit(motes, (struct mote){
			.at = v3_add(body_head(b), v3(b->girth, -b->girth * 1.5f, 0)),
			.velocity = v3(2, -3, 0),
			.colour = SNORE,
			.life = 3,
			.glyph = rng_next() % 2 ? 'z' : 'Z',
		});
	}
}

static void
startle(struct mind *m, const struct senses *s, const struct body *b,
    struct voice *v, struct motes *motes)
{
	vec3 head = body_head(b);
	float away = rng_range(0, TAU);

	if (!s->hearing->jolted || s->hearing->music > 0.1f ||
	    m->since_beat < 2 || m->startle_wait > 0) {
		return;
	}

	m->bolt = BOLT_TIME;
	m->bolt_to = inside(s, v3_add(head, v3(cosf(away) * 25, sinf(away) * 25, 0)),
	    b->girth * 2);
	m->startle_wait = STARTLE_GAP;
	words_say(v, URGENCY_ALARM, SAY_STARTLE, NULL);

	motes_emit(motes, (struct mote){
		.at = v3_add(head, v3(0, -b->girth * 2, 0)),
		.velocity = v3(0, -6, 0),
		.colour = WHITE,
		.life = 0.9f,
		.glyph = '!',
	});
}

static void
hum(const struct senses *s, const struct body *b, struct motes *motes)
{
	const struct hearing *h = s->hearing;

	if (!h->struck || h->music < MUSIC_HEARD) {
		return;
	}
	motes_emit(motes, (struct mote){
		.at = v3_add(body_head(b), v3(0, -b->girth * 1.5f, 0)),
		.velocity = v3(rng_range(-4, 4), -9, 0),
		.colour = v3_lerp(PARTY, WHITE, 0.4f),
		.gravity = -2,
		.life = 1.6f,
		.glyph = rng_next() % 2 ? NOTE : NOTES,
	});
}

static void
react(struct mind *m, const struct senses *s, const struct body *b)
{
	vec3 head = body_head(b), away;

	if (m->bolt > 0) {
		m->target = m->bolt_to;
		m->gait.speed = 30;
		m->gait.turn = 6;
	} else if (m->flee > 0) {
		away = v3_norm(v3_sub(head, m->flee_from));
		m->target = inside(s, v3_add(head, v3_scale(away, 25)), b->girth * 2);
		m->gait.speed = 18;
		m->gait.turn = 4;
	}
}

static void
strain(struct mind *m)
{
	float h = m->heat * m->heat;

	if (m->mood == MOOD_SLEEP) {
		return;
	}
	m->gait.speed *= 1 + 1.2f * h;
	m->gait.tempo *= 1 + m->heat;
	m->gait.jitter += 2.5f * h;

	m->gait.speed *= 1 - 0.35f * smoothstepf(150, 500, m->fullness);
}

static void
grow(struct mind *m, const struct senses *s, struct body *b, float dt)
{
	float girth = clampf(fminf(s->width, s->height) * 0.06f, 1.6f, 5.5f);
	float length = b->girth * (12 + 10 * s->soil->memory);
	float room = (s->width + s->height) * 0.45f;

	if (m->whim == WHIM_STRETCH) {
		length *= 1.35f;
	}
	b->girth = followf(b->girth, girth, 2, dt);
	m->gait.length = fminf(length, room) / BODY_SPACING;
}

static void
swallow(struct mind *m, struct body *b)
{
	static const struct {
		vec3  tint;
		float stain;
	} taste[] = {
		[FLAVOUR_PLAIN] = { { 0.95f, 0.75f, 0.72f }, 0.5f },
		[FLAVOUR_WARN]  = { { 0.95f, 0.65f, 0.20f }, 0.8f },
		[FLAVOUR_ERROR] = { { 1.00f, 0.95f, 0.60f }, 0.9f },
	};

	body_wave(b, (struct wave){
		.s = 0,
		.speed = 0.2f,
		.size = fminf(0.25f + 0.015f * (float)m->digesting, 0.6f),
		.tint = taste[m->richest].tint,
		.stain = taste[m->richest].stain,
	});
	m->digesting = 0;
	m->richest = FLAVOUR_PLAIN;
}

static void
burp(struct mind *m, const struct body *b, struct voice *v,
    struct motes *motes)
{
	vec3 head = body_head(b);
	int i;

	words_say(v, URGENCY_NEWS, SAY_BURP, NULL);
	for (i = 0; i < 3; i++) {
		motes_emit(motes, (struct mote){
			.at = v3_add(head, v3(rng_range(-2, 2), -b->girth, 0)),
			.velocity = v3(rng_range(-2, 2), rng_range(-7, -4), 0),
			.colour = v3(0.7f, 0.8f, 0.7f),
			.life = 1.2f,
			.glyph = i % 2 ? 'o' : DEGREE,
		});
	}
	m->fullness *= 0.6f;
	m->stuffed = false;
}

static void
digest(struct mind *m, const struct body *b, struct voice *v,
    struct motes *motes, float dt)
{
	m->fullness *= expf(-dt / DIGESTION);

	if (!m->stuffed && m->fullness > FULL) {
		m->stuffed = true;
		words_say(v, URGENCY_NEWS, SAY_FULL, NULL);
	} else if (m->stuffed && m->fullness < FULL / 2) {
		m->stuffed = false;
	}
	if (m->fullness > BURSTING) {
		burp(m, b, v, motes);
	}
}

static void
graze(struct mind *m, const struct senses *s, struct body *b, struct voice *v)
{
	int eaten = leaves_eat(s->leaves, body_head(b), b->girth * 0.9f);

	if (eaten == 0) {
		return;
	}
	body_wave(b, (struct wave){
		.s = 0, .speed = 0.25f, .size = 0.4f, .tint = LEAFY, .stain = 0.9f,
	});
	m->fullness += LEAF_MEAL * (float)eaten;
	m->joy += 0.5f;
	words_say(v, URGENCY_NEWS, SAY_LEAF, NULL);
}

static void
feed(struct mind *m, const struct senses *s, struct body *b, struct voice *v,
    struct motes *motes, float dt)
{
	struct meal meal;
	const struct bite *bite;
	int i;

	food_eat(s->food, body_head(b), b->girth * 0.9f, &meal);

	for (i = 0; i < meal.count; i++) {
		bite = &meal.bites[i];
		motes_emit(motes, (struct mote){
			.at = bite->at,
			.velocity = v3(rng_range(-6, 6), rng_range(-10, -3), 0),
			.colour = food_colour(bite->flavour),
			.gravity = 30,
			.life = 0.5f,
			.glyph = bite->glyph,
		});
		if (bite->flavour > m->richest) {
			m->richest = bite->flavour;
		}
	}

	if (meal.errors > 0) {
		if (m->spice < 0.5f) {
			words_say(v, URGENCY_NEWS, SAY_SPICY, NULL);
		}
		m->spice = 1;
	} else if (meal.warnings > 0 && m->richest == FLAVOUR_WARN) {
		words_say(v, URGENCY_IDLE, SAY_WARNING, NULL);
	}

	m->digesting += meal.count;
	m->fullness += (float)meal.count;
	m->gulp_wait -= dt;
	if ((meal.lines > 0 || m->digesting >= MOUTHFUL) && m->gulp_wait <= 0) {
		swallow(m, b);
		m->gulp_wait = GULP_GAP;
	}

	m->overfed_wait -= dt;
	if (s->food->skipped > m->skipped && m->overfed_wait <= 0) {
		words_say(v, URGENCY_NEWS, SAY_FLOOD, NULL);
		m->overfed_wait = OVERFED_GAP;
	}
	m->skipped = s->food->skipped;

	graze(m, s, b, v);
}

static void
ripple(struct mind *m, const struct senses *s, struct body *b, float dt)
{
	m->inflow = fminf(m->inflow + s->soil->inflow * dt, RIPPLE_BYTES * 2);
	m->outflow = fminf(m->outflow + s->soil->outflow * dt, RIPPLE_BYTES * 2);
	m->ripple_wait -= dt;

	if (m->ripple_wait > 0) {
		return;
	}
	if (m->inflow >= RIPPLE_BYTES) {
		m->inflow -= RIPPLE_BYTES;
		m->ripple_wait = RIPPLE_GAP;
		body_wave(b, (struct wave){ .s = -0.05f, .speed = 1.1f, .size = 0.12f });
	} else if (m->outflow >= RIPPLE_BYTES) {
		m->outflow -= RIPPLE_BYTES;
		m->ripple_wait = RIPPLE_GAP;
		body_wave(b, (struct wave){ .s = 1.05f, .speed = -1.1f, .size = 0.12f });
	}
}

static void
unwind(struct mind *m, const struct senses *s, float dt)
{
	m->since_beat = s->hearing->struck ? 0 : m->since_beat + dt;
	m->joy = fmaxf(m->joy - 0.6f * dt, 0);
	m->flee -= dt;
	m->bolt -= dt;
	m->startle_wait -= dt;
	m->grumble -= dt;
	m->blink -= dt;

	m->blink_wait -= dt;
	if (m->blink_wait <= 0) {
		m->blink = 0.15f;
		m->blink_wait = rng_range(2.5f, 6);
	}
}

static enum eyes
gaze(const struct mind *m)
{
	if (settled(m) > 0.6f || m->blink > 0) {
		return EYES_SHUT;
	}
	if (m->whim == WHIM_STARE || m->bolt > 0) {
		return EYES_WIDE;
	}
	return EYES_OPEN;
}

static void
appear(struct mind *m, const struct senses *s, const struct body *b, float dt)
{
	const struct hearing *h = s->hearing;
	struct look *look = &m->look;
	float rest = settled(m);
	float pulse = h->beat * h->music;
	float glee = fminf(m->joy, 1);
	float undulate, tempo;
	vec3 colour;

	colour = v3_lerp(CALM, HOT, clampf(m->heat + 0.35f * m->spice, 0, 1));
	colour = v3_lerp(colour, PARTY, h->music * (0.25f + 0.5f * h->treble));
	look->colour = v3_scale(colour, lerpf(1, 0.55f, rest));

	switch (m->mood) {
	case MOOD_DANCE:
		undulate = b->girth * (0.35f + 0.9f * h->bass);
		tempo = h->tempo > 0 ? h->tempo / 120 : 1.2f;
		tempo += 1.5f * pulse;
		break;
	case MOOD_SLEEP:
		undulate = 0;
		tempo = 0.15f;
		break;
	case MOOD_EAT:
		undulate = b->girth * 0.08f;
		tempo = 0.8f;
		break;
	case MOOD_WANDER:
	default:
		undulate = b->girth * 0.12f;
		tempo = 0.6f;
		break;
	}
	undulate += b->girth * 0.35f * glee;

	look->undulate = followf(look->undulate, undulate, 3, dt);
	look->phase = fmodf(look->phase + TAU * tempo * dt, TAU * 4);
	look->roll = b->girth * (m->mood == MOOD_DANCE ? 0.6f : 0.35f);
	look->glow = fmaxf(pulse, fmaxf(0.5f * glee, m->bolt > 0 ? 1.0f : 0));
	look->swell = 0.22f * pulse + 0.08f * glee +
	    0.06f * rest * sinf(TAU * m->nap / 4.5f);
	look->eyes = gaze(m);
}

static void
hearts(const struct body *b, vec3 at, struct motes *motes)
{
	int i;

	for (i = 0; i < 2; i++) {
		motes_emit(motes, (struct mote){
			.at = v3_add(at, v3(rng_range(-1, 1), -b->girth, 0)),
			.velocity = v3(rng_range(-3, 3), rng_range(-9, -5), 0),
			.colour = PINK,
			.gravity = 2,
			.life = 1.4f,
			.glyph = HEART,
		});
	}
}

static bool
touches(const struct body *b, vec3 at, float reach)
{
	int i;

	for (i = 1; i < b->length; i++) {
		if (v3_len(v3_sub(b->spine[i], at)) < reach) {
			return true;
		}
	}
	return false;
}

static void
key(struct mind *m, uint32_t k, const struct senses *s, const struct body *b,
    struct voice *v)
{
	char name[8] = "that";

	if (k == ' ') {
		leaves_drop(s->leaves, somewhere(s, b->girth));
		return;
	}
	if (k == '?') {
		words_say(v, URGENCY_REPLY, SAY_HELP, NULL);
		return;
	}
	if (m->grumble > 0) {
		return;
	}
	m->grumble = GRUMBLE_GAP;
	if (k > ' ' && k < 0x7f) {
		snprintf(name, sizeof name, "%c", (char)k);
	}
	words_say(v, URGENCY_REPLY, SAY_UNKNOWN_KEY, name);
}

static void
click(struct mind *m, vec3 at, const struct senses *s, const struct body *b,
    struct voice *v, struct motes *motes)
{
	float reach = b->girth * 1.2f;
	bool head = v3_len(v3_sub(body_head(b), at)) < reach;

	if (!head && !touches(b, at, reach)) {
		leaves_drop(s->leaves, inside(s, at, 1));
		return;
	}

	if (m->mood == MOOD_SLEEP) {
		words_say(v, URGENCY_REPLY, SAY_GRUMPY, NULL);
		return;
	}

	if (head) {
		m->blink = 0.35f;
		m->joy += 0.5f;
		words_say(v, URGENCY_REPLY, SAY_BOOP, NULL);
		return;
	}

	m->joy += 1;
	hearts(b, at, motes);
	if (m->joy > TOO_MANY_PETS) {
		m->joy = 0;
		m->flee = FLEE_TIME;
		m->flee_from = at;
		words_say(v, URGENCY_REPLY, SAY_ENOUGH, NULL);
	} else {
		words_say(v, URGENCY_REPLY, SAY_PET, NULL);
	}
}

void
mind_press(struct mind *m, const struct press *p, const struct senses *s,
    const struct body *b, struct voice *v, struct motes *motes)
{
	if (p->kind == PRESS_KEY) {
		key(m, p->key, s, b, v);
	} else {
		click(m, v3((float)p->x + 0.5f, ((float)p->y + 0.5f) * UNITS_PER_ROW, 0),
		    s, b, v, motes);
	}
}

static bool
hunger(const struct senses *s, vec3 head, vec3 *meal)
{
	vec3 text, leaf;
	bool reading = food_nearest(s->food, head, &text);
	bool grazing = leaves_nearest(s->leaves, head, &leaf);

	if (grazing && (!reading ||
	    v3_len(v3_sub(leaf, head)) < v3_len(v3_sub(text, head)))) {
		*meal = leaf;
		return true;
	}
	*meal = text;
	return reading;
}

void
mind_think(struct mind *m, const struct senses *s, struct body *b,
    struct voice *v, struct motes *motes, float dt)
{
	vec3 meal;
	bool hungry = hunger(s, body_head(b), &meal);

	feel(m, s, hungry, dt);
	startle(m, s, b, v, motes);
	choose(m, s, v, hungry);

	switch (m->mood) {
	case MOOD_EAT:
		eat(m, s, b, meal, dt);
		break;
	case MOOD_DANCE:
		dance(m, s, b, v, dt);
		break;
	case MOOD_SLEEP:
		doze(m, b, motes, dt);
		break;
	case MOOD_WANDER:
		wander(m, s, b, v, dt);
		break;
	}

	react(m, s, b);
	hum(s, b, motes);
	strain(m);
	grow(m, s, b, dt);
	feed(m, s, b, v, motes, dt);
	digest(m, b, v, motes, dt);
	ripple(m, s, b, dt);
	unwind(m, s, dt);
	notice(m, s, v, dt);
	appear(m, s, b, dt);
}
