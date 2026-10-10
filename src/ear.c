#include <complex.h>
#include <string.h>

#include "capture.h"
#include "ear.h"
#include "vec.h"

#define WINDOW      1024
#define WINDOW_BITS 10
#define BINS        (WINDOW / 2)
#define ONSETS      48
#define BEATS       12

#define BASS_TOP    150.0f
#define MID_TOP     2000.0f
#define FLUX_TOP    2500.0f
#define KICK_TOP    300.0f
#define ABOVE_KICK  0.35f

#define SILENCE     (-50.0f)
#define FLOOR_RISE  0.1f
#define PEAK_FALL   3.0f
#define SPAN        12.0f
#define BEAT_GAP    0.28f
#define SLOWEST     85.0f
#define FASTEST     170.0f
#define RHYTHM      5.0f
#define STEADY      0.2f
#define JOLT        20.0f
#define SETTLING    1.0f

_Static_assert(WINDOW == 1 << WINDOW_BITS, "window and its bits disagree");

struct gauge {
	float floor;
	float peak;
};

static struct ring ring;
static bool listening;
static float rate;

static float complex twiddle[BINS];
static unsigned reversed[WINDOW];
static float hann[WINDOW];

static struct gauge loudness, bass, mid, treble;
static float before[BINS];
static float onsets[ONSETS];
static int onset_next;
static float beats[BEATS];
static int beat_next;
static float clock, since_beat, hush = -120;

static unsigned
reverse(unsigned v)
{
	unsigned r = 0;
	int i;

	for (i = 0; i < WINDOW_BITS; i++) {
		r = (r << 1) | (v & 1);
		v >>= 1;
	}
	return r;
}

static void
fft(float complex *x)
{
	float complex t, swap;
	int size, half, stride, start, k;
	unsigned i;

	for (i = 0; i < WINDOW; i++) {
		if (i < reversed[i]) {
			swap = x[i];
			x[i] = x[reversed[i]];
			x[reversed[i]] = swap;
		}
	}

	for (size = 2; size <= WINDOW; size *= 2) {
		half = size / 2;
		stride = WINDOW / size;
		for (start = 0; start < WINDOW; start += size) {
			for (k = 0; k < half; k++) {
				t = twiddle[k * stride] * x[start + k + half];
				x[start + k + half] = x[start + k] - t;
				x[start + k] += t;
			}
		}
	}
}

static float
decibels(float power)
{
	return 10.0f * log10f(power + 1e-12f);
}

static float
gauge_read(struct gauge *g, float db, float dt)
{
	g->floor = db < g->floor ? db : g->floor + FLOOR_RISE * dt;
	g->peak = db > g->peak ? db : g->peak - PEAK_FALL * dt;

	return clampf((db - g->floor) / fmaxf(g->peak - g->floor, SPAN), 0, 1);
}

int
ear_open(void)
{
	unsigned i;

	for (i = 0; i < BINS; i++) {
		twiddle[i] = cexpf(-I * (TAU * (float)i / WINDOW));
	}
	for (i = 0; i < WINDOW; i++) {
		reversed[i] = reverse(i);
		hann[i] = 0.5f - 0.5f * cosf(TAU * (float)i / (WINDOW - 1));
	}

	loudness = bass = mid = treble = (struct gauge){ 0, -120 };

	if (capture_start(&ring, &rate) < 0) {
		return -1;
	}
	listening = true;
	return 0;
}

void
ear_close(void)
{
	if (listening) {
		capture_stop();
		listening = false;
	}
}

static float
spread(const float *history, int count, float *mean)
{
	float sum = 0, square = 0;
	int i;

	for (i = 0; i < count; i++) {
		sum += history[i];
		square += history[i] * history[i];
	}
	*mean = sum / (float)count;
	return sqrtf(fmaxf(square / (float)count - *mean * *mean, 0));
}

static float
median(float *values, int count)
{
	float v;
	int i, j;

	for (i = 1; i < count; i++) {
		v = values[i];
		for (j = i; j > 0 && values[j - 1] > v; j--) {
			values[j] = values[j - 1];
		}
		values[j] = v;
	}
	return values[count / 2];
}

static bool
near(float value, float target)
{
	return fabsf(value - target) < STEADY * target;
}

static float
fold(float bpm)
{
	while (bpm > FASTEST) {
		bpm /= 2;
	}
	while (bpm < SLOWEST) {
		bpm *= 2;
	}
	return bpm;
}

static float
tempo(void)
{
	float gaps[BEATS], later = 0, typical;
	int i, at, count = 0, fit = 0;

	for (i = 1; i <= BEATS; i++) {
		at = (beat_next - i + BEATS) % BEATS;
		if (beats[at] <= 0 || clock - beats[at] > RHYTHM) {
			break;
		}
		if (i > 1) {
			gaps[count++] = later - beats[at];
		}
		later = beats[at];
	}
	if (count < 4) {
		return 0;
	}

	typical = median(gaps, count);
	for (i = 0; i < count; i++) {
		fit += near(gaps[i], typical) || near(gaps[i], typical * 2);
	}
	return fit * 4 >= count * 3 ? fold(60 / typical) : 0;
}

void
ear_listen(struct hearing *h, float dt)
{
	float samples[WINDOW];
	float complex spectrum[WINDOW];
	float power = 0, low = 0, middle = 0, high = 0, flux = 0;
	float hz, magnitude, mean, deviation, db, bpm;
	bool present;
	int i;

	clock += dt;
	since_beat += dt;
	h->struck = false;
	h->jolted = false;
	h->beat *= expf(-6.0f * dt);

	if (!listening) {
		h->level = h->bass = h->mid = h->treble = 0;
		h->music = followf(h->music, 0, 0.4f, dt);
		return;
	}

	ring_latest(&ring, samples, WINDOW);
	for (i = 0; i < WINDOW; i++) {
		power += samples[i] * samples[i];
		spectrum[i] = samples[i] * hann[i];
	}
	power /= WINDOW;
	fft(spectrum);

	for (i = 1; i < BINS; i++) {
		hz = (float)i * rate / WINDOW;
		magnitude = cabsf(spectrum[i]);

		if (hz < BASS_TOP) {
			low += magnitude * magnitude;
		} else if (hz < MID_TOP) {
			middle += magnitude * magnitude;
		} else {
			high += magnitude * magnitude;
		}

		if (hz < FLUX_TOP) {
			magnitude = logf(1 + 10 * magnitude);
			flux += fmaxf(magnitude - before[i], 0) *
			    (hz < KICK_TOP ? 1 : ABOVE_KICK);
			before[i] = magnitude;
		}
	}

	db = decibels(power);
	h->level = gauge_read(&loudness, db, dt);
	h->bass = gauge_read(&bass, decibels(low), dt);
	h->mid = gauge_read(&mid, decibels(middle), dt);
	h->treble = gauge_read(&treble, decibels(high), dt);

	present = db > SILENCE;

	h->jolted = clock > SETTLING && present && db - hush > JOLT;
	hush = followf(hush, db, 1.0f, dt);

	deviation = spread(onsets, ONSETS, &mean);
	onsets[onset_next] = flux;
	onset_next = (onset_next + 1) % ONSETS;

	if (present && flux > mean + 1.5f * deviation && since_beat > BEAT_GAP) {
		h->struck = true;
		h->beat = 1;
		since_beat = 0;
		beats[beat_next] = clock;
		beat_next = (beat_next + 1) % BEATS;
	}

	bpm = present ? tempo() : 0;
	if (bpm > 0) {
		h->tempo = bpm;
	}
	h->music = bpm > 0 ?
	    followf(h->music, 1, 0.6f, dt) :
	    followf(h->music, 0, 0.4f, dt);
}
