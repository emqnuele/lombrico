#ifndef LOMBRICO_RNG_H
#define LOMBRICO_RNG_H

#include <stdint.h>

void     rng_seed(uint64_t seed);
uint32_t rng_next(void);
float    rng_unit(void);
float    rng_range(float lo, float hi);

#endif
