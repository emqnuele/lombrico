#ifndef LOMBRICO_VEC_H
#define LOMBRICO_VEC_H

#include <math.h>

#define TAU 6.28318530718f

typedef struct {
	float x, y, z;
} vec3;

static inline vec3
v3(float x, float y, float z)
{
	return (vec3){ x, y, z };
}

static inline vec3
v3_add(vec3 a, vec3 b)
{
	return v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline vec3
v3_sub(vec3 a, vec3 b)
{
	return v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline vec3
v3_scale(vec3 a, float k)
{
	return v3(a.x * k, a.y * k, a.z * k);
}

static inline vec3
v3_mul(vec3 a, vec3 b)
{
	return v3(a.x * b.x, a.y * b.y, a.z * b.z);
}

static inline float
v3_dot(vec3 a, vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3
v3_cross(vec3 a, vec3 b)
{
	return v3(a.y * b.z - a.z * b.y,
	    a.z * b.x - a.x * b.z,
	    a.x * b.y - a.y * b.x);
}

static inline float
v3_len(vec3 a)
{
	return sqrtf(v3_dot(a, a));
}

static inline vec3
v3_norm(vec3 a)
{
	float len = v3_len(a);

	return len > 1e-6f ? v3_scale(a, 1.0f / len) : a;
}

static inline vec3
v3_lerp(vec3 a, vec3 b, float t)
{
	return v3_add(a, v3_scale(v3_sub(b, a), t));
}

static inline float
clampf(float x, float lo, float hi)
{
	return x < lo ? lo : x > hi ? hi : x;
}

static inline float
lerpf(float a, float b, float t)
{
	return a + (b - a) * t;
}

static inline float
smoothstepf(float edge0, float edge1, float x)
{
	float t = clampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);

	return t * t * (3.0f - 2.0f * t);
}

static inline float
followf(float from, float to, float rate, float dt)
{
	return to + (from - to) * expf(-rate * dt);
}

static inline vec3
v3_follow(vec3 from, vec3 to, float rate, float dt)
{
	return v3_lerp(to, from, expf(-rate * dt));
}

static inline float
wrap_angle(float a)
{
	return a - TAU * floorf((a + TAU / 2) / TAU);
}

#endif
