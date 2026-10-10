#include "rng.h"

static uint64_t state = 0x853c49e6748fea9bULL;
static const uint64_t stream = 0xda3e39cb94b95bdbULL;

void
rng_seed(uint64_t seed)
{
	state = 0;
	rng_next();
	state += seed;
	rng_next();
}

uint32_t
rng_next(void)
{
	uint64_t old = state;
	uint32_t xorshifted = (uint32_t)(((old >> 18) ^ old) >> 27);
	uint32_t rot = (uint32_t)(old >> 59);

	state = old * 6364136223846793005ULL + stream;

	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

float
rng_unit(void)
{
	return (float)(rng_next() >> 8) * 0x1.0p-24f;
}

float
rng_range(float lo, float hi)
{
	return lo + (hi - lo) * rng_unit();
}
