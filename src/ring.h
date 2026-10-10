#ifndef LOMBRICO_RING_H
#define LOMBRICO_RING_H

#include <stdatomic.h>
#include <stdint.h>

#define RING_SIZE 8192u
#define RING_MASK (RING_SIZE - 1)

_Static_assert((RING_SIZE & RING_MASK) == 0, "ring size: a power of two");

struct ring {
	float            samples[RING_SIZE];
	_Atomic uint32_t written;
};

static inline void
ring_push(struct ring *r, const float *samples, uint32_t count)
{
	uint32_t at = atomic_load_explicit(&r->written, memory_order_relaxed);
	uint32_t i;

	for (i = 0; i < count; i++) {
		r->samples[(at + i) & RING_MASK] = samples[i];
	}
	atomic_store_explicit(&r->written, at + count, memory_order_release);
}

static inline void
ring_latest(struct ring *r, float *samples, uint32_t count)
{
	uint32_t at = atomic_load_explicit(&r->written, memory_order_acquire);
	uint32_t i;

	for (i = 0; i < count; i++) {
		samples[i] = r->samples[(at - count + i) & RING_MASK];
	}
}

#endif
