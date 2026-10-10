#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "body.h"
#include "canvas.h"
#include "ear.h"
#include "food.h"
#include "hud.h"
#include "leaves.h"
#include "mind.h"
#include "motes.h"
#include "paint.h"
#include "rng.h"
#include "skin.h"
#include "soil.h"
#include "term.h"
#include "tick.h"
#include "voice.h"

#define VERSION        "1.0.0"
#define FRAME          (1.0 / 30)
#define SAMPLE_EVERY   0.5
#define LONGEST_STEP   0.1f
#define BIRTH_LENGTH   40

static const char usage[] =
    "usage: lombrico [-d] [-h] [-v]\n"
    "\n"
    "a worm that lives in your terminal.\n"
    "pipe anything into it and it eats the output: make 2>&1 | lombrico\n"
    "\n"
    "  -d, --deaf     never open the microphone\n"
    "  -h, --help     show this and leave\n"
    "  -v, --version  show the version and leave\n"
    "\n"
    "keys: q quit, i vitals, m deafen, space leaf, ? help\n"
    "mouse: click its head, pet its body, drop leaves anywhere else\n";

static struct canvas  canvas;
static struct soil    soil;
static struct hearing hearing, silence;
static struct food    food;
static struct body    body;
static struct mind    mind;
static struct voice   voice;
static struct motes   motes;
static struct leaves  leaves;

static bool deaf, vitals, leaving;

static int
parse(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--deaf")) {
			deaf = true;
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			fputs(usage, stdout);
			return 0;
		} else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
			puts("lombrico " VERSION);
			return 0;
		} else {
			fprintf(stderr, "lombrico: unknown option %s\n\n%s", argv[i], usage);
			return 2;
		}
	}
	return -1;
}

static struct senses
sense(void)
{
	time_t now = time(NULL);
	const struct tm *local = localtime(&now);

	return (struct senses){
		.soil = &soil,
		.hearing = deaf ? &silence : &hearing,
		.food = &food,
		.leaves = &leaves,
		.width = (float)canvas.width,
		.height = (float)canvas.height * UNITS_PER_ROW,
		.hour = local != NULL ? local->tm_hour : 12,
	};
}

static bool
touch(const struct senses *senses)
{
	struct press press;
	bool touched = false;

	while (term_press(&press)) {
		touched = true;
		if (press.kind == PRESS_KEY && (press.key == 'q' || press.key == 'Q')) {
			leaving = true;
		} else if (press.kind == PRESS_KEY && press.key == 'i') {
			vitals = !vitals;
		} else if (press.kind == PRESS_KEY && press.key == 'm') {
			deaf = !deaf;
		} else {
			mind_press(&mind, &press, senses, &body, &voice, &motes);
		}
	}
	return touched;
}

static int
fit(void)
{
	int width, height;

	term_size(&width, &height);
	return canvas_resize(&canvas, width, height);
}

static void
live(const struct senses *senses, float dt)
{
	food_place(&food, canvas.width, canvas.height - 1 - vitals);
	leaves_age(&leaves, dt);
	mind_think(&mind, senses, &body, &voice, &motes, dt);
	body_crawl(&body, &mind.gait, mind.target, dt);
	body_flow(&body, dt);
	motes_drift(&motes, dt);
	voice_age(&voice, dt);
}

static int
draw(void)
{
	canvas_clear(&canvas);
	food_draw(&food, &canvas);
	leaves_draw(&leaves, &canvas);
	skin_draw(&canvas, &body, &mind.look);
	motes_draw(&motes, &canvas);
	voice_draw(&voice, &canvas, body_head(&body), body.girth);
	if (vitals) {
		hud_draw(&canvas, &soil, deaf ? &silence : &hearing, &food);
	}
	return paint(&canvas);
}

static void
be_born(void)
{
	float x = (float)canvas.width / 2;
	float y = (float)canvas.height * UNITS_PER_ROW / 2;

	body_init(&body, v3(x + BIRTH_LENGTH / 2, y, 0), BIRTH_LENGTH, 2);
	mind_init(&mind);
	mind.target = body_head(&body);
	mind_greet(&voice, sense().hour);
}

int
main(int argc, char **argv)
{
	double now, last, next, sampled;
	int status = parse(argc, argv);
	struct senses senses;

	if (status >= 0) {
		return status;
	}

	rng_seed((uint64_t)time(NULL) ^ (uint64_t)(uintptr_t)&argc);

	food_open(&food);
	if (term_open() < 0) {
		fputs("lombrico: there is no terminal to live in\n", stderr);
		return 1;
	}
	if (fit() < 0) {
		term_close();
		fputs("lombrico: out of memory\n", stderr);
		return 1;
	}
	soil_open(&soil);
	if (!deaf) {
		ear_open();
	}

	be_born();
	last = sampled = next = tick_now();
	status = 0;

	while (!leaving && !term_quitting()) {
		next += FRAME;
		tick_sleep(next - tick_now());
		now = tick_now();
		if (now - next > 0.25) {
			next = now;
		}

		if (term_resized() && fit() < 0) {
			status = 1;
			break;
		}
		senses = sense();
		senses.touched = touch(&senses);
		food_read(&food, (float)(now - last));
		if (now - sampled >= SAMPLE_EVERY) {
			soil_sample(&soil, (float)(now - sampled));
			sampled = now;
		}
		ear_listen(&hearing, (float)(now - last));

		live(&senses, fminf((float)(now - last), LONGEST_STEP));
		last = now;

		if (draw() < 0) {
			status = 1;
			break;
		}
	}

	ear_close();
	term_close();
	paint_free();
	canvas_free(&canvas);

	if (status != 0) {
		fputs("lombrico: out of memory\n", stderr);
	}
	return status;
}
