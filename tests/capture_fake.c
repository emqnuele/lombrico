#include "capture_fake.h"

#define RATE 44100

static struct ring *target;

int
capture_start(struct ring *ring, float *rate)
{
	target = ring;
	*rate = RATE;
	return 0;
}

void
capture_stop(void)
{
	target = NULL;
}

void
capture_feed(const float *samples, uint32_t count)
{
	ring_push(target, samples, count);
}
