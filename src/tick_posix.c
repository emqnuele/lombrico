#include <errno.h>
#include <time.h>

#include "tick.h"

double
tick_now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

void
tick_sleep(double seconds)
{
	struct timespec left;

	if (seconds <= 0) {
		return;
	}
	left.tv_sec = (time_t)seconds;
	left.tv_nsec = (long)((seconds - (double)left.tv_sec) * 1e9);

	while (nanosleep(&left, &left) < 0 && errno == EINTR) {
	}
}
