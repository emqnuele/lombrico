#include <string.h>

#include "stream_fake.h"

static const char *pending;
static size_t left;

void
stream_give(const char *bytes)
{
	pending = bytes;
	left = strlen(bytes);
}

bool
stream_fed(void)
{
	return true;
}

bool
stream_piped_out(void)
{
	return false;
}

long
stream_read(char *bytes, size_t cap)
{
	size_t n = left < cap ? left : cap;

	if (n == 0) {
		return STREAM_ENDED;
	}
	memcpy(bytes, pending, n);
	pending += n;
	left -= n;
	return (long)n;
}

bool
stream_pass(const char *bytes, size_t len)
{
	(void)bytes;
	(void)len;
	return true;
}
