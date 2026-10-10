#include <stdio.h>

#include "../src/ear.h"
#include "../src/rng.h"
#include "../src/vec.h"
#include "capture_fake.h"

#define RATE   44100
#define FRAME  (RATE / 30)

enum scene {
	SCENE_QUIET,
	SCENE_SONG,
	SCENE_TINNY,
	SCENE_CLATTER,
};

static float
kick(float since)
{
	return 0.5f * expf(-since * 18) *
	    sinf(TAU * (55 + 90 * expf(-since * 30)) * since);
}

static float
tick(float since, long n)
{
	static float before;
	float noise = rng_range(-1, 1), high = noise - before;

	(void)n;
	before = noise;
	return 0.25f * expf(-since * 40) * high;
}

static float
room(long n, float bpm, enum scene scene)
{
	static float knock, next_knock;
	float t = (float)n / RATE;
	float hum = 0.002f * rng_range(-1, 1);
	float beat = 60.0f / bpm;
	float since = fmodf(t, beat);
	float hat;

	switch (scene) {
	case SCENE_QUIET:
		return hum;
	case SCENE_CLATTER:
		if (t >= next_knock) {
			knock = next_knock;
			next_knock = t + rng_range(0.3f, 1.2f);
		}
		return hum + kick(t - knock);
	case SCENE_TINNY:
		return hum + tick(since, n) + 0.05f * sinf(TAU * 880 * t);
	case SCENE_SONG:
		break;
	}

	hat = 0.05f * expf(-fmodf(t + beat / 2, beat) * 60) * rng_range(-1, 1);
	return hum + kick(since) + hat + 0.05f * sinf(TAU * 220 * t);
}

static int
listen_for(float seconds, float bpm, enum scene scene, struct hearing *h,
    long *n)
{
	float samples[FRAME];
	int frames = (int)(seconds * 30), beats = 0, f, i;

	for (f = 0; f < frames; f++) {
		for (i = 0; i < FRAME; i++) {
			samples[i] = room((*n)++, bpm, scene);
		}
		capture_feed(samples, FRAME);
		ear_listen(h, 1.0f / 30);
		beats += h->struck;
	}
	return beats;
}

static int failures;

static void
expect(bool ok, const char *what, double value)
{
	printf("%s  %-40s %.2f\n", ok ? "ok  " : "FAIL", what, value);
	failures += !ok;
}

int
main(void)
{
	struct hearing h = { 0 };
	long n = 0;
	int beats;

	rng_seed(7);
	ear_open();

	listen_for(6, 120, SCENE_SONG, &h, &n);
	expect(h.music > 0.8f, "a song playing from the start is music",
	    (double)h.music);

	listen_for(8, 120, SCENE_QUIET, &h, &n);
	expect(h.music < 0.1f, "silence ends the music", (double)h.music);

	beats = listen_for(10, 120, SCENE_QUIET, &h, &n);
	expect(beats <= 2, "a quiet room has no beats", beats);

	beats = listen_for(8, 120, SCENE_SONG, &h, &n);
	expect(h.music > 0.8f, "120 bpm is music", (double)h.music);
	expect(beats >= 12 && beats <= 17, "120 bpm gives ~16 beats in 8 s", beats);
	expect(fabsf(h.tempo - 120) < 8, "and reads 120", (double)h.tempo);
	expect(h.bass > 0.25f, "a kick drum is bass", (double)h.bass);

	listen_for(8, 90, SCENE_SONG, &h, &n);
	expect(fabsf(h.tempo - 90) < 6, "90 bpm with offbeat hats reads 90",
	    (double)h.tempo);

	listen_for(8, 160, SCENE_SONG, &h, &n);
	expect(fabsf(h.tempo - 160) < 10, "160 bpm reads 160", (double)h.tempo);

	listen_for(8, 120, SCENE_QUIET, &h, &n);
	listen_for(8, 120, SCENE_TINNY, &h, &n);
	expect(h.music > 0.8f, "a song with no bass is still music",
	    (double)h.music);
	expect(fabsf(h.tempo - 120) < 8, "and still keeps time", (double)h.tempo);

	listen_for(8, 120, SCENE_QUIET, &h, &n);
	listen_for(12, 120, SCENE_CLATTER, &h, &n);
	expect(h.music < 0.5f, "knocks at random times are not music",
	    (double)h.music);

	ear_close();
	return failures != 0;
}
