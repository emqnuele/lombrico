#include "skin.h"

#define RAMP        ".:-=+*#%@"
#define RAMP_TOP    ((int)sizeof RAMP - 2)
#define SUBSTEPS    3
#define SAMPLE_GAP  0.35f
#define TAPER_FROM  0.35f
#define TAIL_GIRTH  0.4f
#define AMBIENT     0.06f

static const vec3 LIGHT = { -0.4207f, -0.6211f, 0.6612f };
static const vec3 HALF  = { -0.2308f, -0.3407f, 0.9114f };
static const vec3 EYE   = { 0.96f, 0.95f, 0.90f };

struct section {
	vec3  centre;
	vec3  along;
	vec3  normal;
	vec3  binormal;
	vec3  ink;
	float radius;
};

static struct section sections[BODY_SEGMENTS];

static float
taper(float s)
{
	return 1.0f - (1.0f - TAIL_GIRTH) * smoothstepf(TAPER_FROM, 1.0f, s);
}

static void
shape(const struct body *b, const struct look *look)
{
	struct section *sec;
	vec3 ahead, behind, along, side, tint;
	float s, sway, swell, stain;
	int i, last = b->length - 1;

	for (i = 0; i <= last; i++) {
		sec = &sections[i];
		s = (float)i / (float)last;

		ahead = b->spine[i > 0 ? i - 1 : 0];
		behind = b->spine[i < last ? i + 1 : last];
		along = v3_norm(v3_sub(ahead, behind));
		side = v3(-along.y, along.x, 0);

		sway = sinf(look->phase - s * TAU * 1.5f) * look->undulate *
		    (0.3f + 0.7f * s);
		sec->centre = v3_add(b->spine[i], v3_scale(side, sway));
		sec->centre.z = sinf(look->phase * 0.5f + s * TAU * 0.8f) * look->roll;

		swell = body_swelling(b, s, &tint, &stain);
		sec->radius = b->girth * taper(s) * (1 + look->swell + swell);
		sec->ink = v3_lerp(look->colour, tint, stain);
	}
}

static void
frame(int length)
{
	struct section *sec, *prev;
	vec3 normal;
	int i, last = length - 1;

	for (i = 0; i <= last; i++) {
		sections[i].along = v3_norm(v3_sub(
		    sections[i > 0 ? i - 1 : 0].centre,
		    sections[i < last ? i + 1 : last].centre));
	}

	sec = &sections[0];
	normal = v3_cross(sec->along, v3(0, 0, 1));
	sec->normal = v3_len(normal) > 1e-4f ? v3_norm(normal) : v3(0, 1, 0);
	sec->binormal = v3_cross(sec->along, sec->normal);

	for (i = 1; i <= last; i++) {
		sec = &sections[i];
		prev = &sections[i - 1];
		normal = v3_sub(prev->normal,
		    v3_scale(sec->along, v3_dot(prev->normal, sec->along)));
		sec->normal = v3_len(normal) > 1e-4f ? v3_norm(normal) : prev->normal;
		sec->binormal = v3_cross(sec->along, sec->normal);
	}
}

static struct section
blend(const struct section *a, const struct section *b, float t)
{
	return (struct section){
		.centre = v3_lerp(a->centre, b->centre, t),
		.along = v3_norm(v3_lerp(a->along, b->along, t)),
		.normal = v3_norm(v3_lerp(a->normal, b->normal, t)),
		.binormal = v3_norm(v3_lerp(a->binormal, b->binormal, t)),
		.ink = v3_lerp(a->ink, b->ink, t),
		.radius = lerpf(a->radius, b->radius, t),
	};
}

static void
splat(struct canvas *c, vec3 at, vec3 normal, vec3 ink, float glow)
{
	float diffuse = fmaxf(v3_dot(normal, LIGHT), 0);
	float shine = powf(fmaxf(v3_dot(normal, HALF), 0), 24);
	float light = clampf(AMBIENT + 0.8f * diffuse + 0.6f * shine + 0.25f * glow,
	    0, 1);
	vec3 colour = v3_add(v3_scale(ink, 0.2f + 0.8f * light),
	    v3_scale(v3(1, 1, 1), 0.35f * shine));

	canvas_point(c, at, at.z, (uint32_t)RAMP[(int)lrintf(light * RAMP_TOP)],
	    colour);
}

static void
ring(struct canvas *c, const struct section *sec, vec3 axis, float lean,
    float glow)
{
	float radius = sec->radius * cosf(lean);
	int count = (int)clampf(ceilf(TAU * radius / SAMPLE_GAP), 6, 256);
	vec3 around, normal;
	float theta;
	int j;

	for (j = 0; j < count; j++) {
		theta = TAU * (float)j / (float)count;
		around = v3_add(v3_scale(sec->normal, cosf(theta)),
		    v3_scale(sec->binormal, sinf(theta)));
		normal = v3_add(v3_scale(around, cosf(lean)),
		    v3_scale(axis, sinf(lean)));
		splat(c, v3_add(sec->centre, v3_scale(normal, sec->radius)),
		    normal, sec->ink, glow);
	}
}

static void
cap(struct canvas *c, const struct section *sec, vec3 axis, float glow)
{
	int count = (int)ceilf(sec->radius * (TAU / 4) / SAMPLE_GAP);
	int k;

	for (k = 1; k <= count; k++) {
		ring(c, sec, axis, (TAU / 4) * (float)k / (float)count, glow);
	}
}

static void
eyes(struct canvas *c, const struct section *head, enum eyes eyes)
{
	static const uint32_t glyph[] = {
		[EYES_OPEN] = 'o',
		[EYES_SHUT] = '-',
		[EYES_WIDE] = 'O',
	};

	vec3 up = v3(0, 0, 1);
	vec3 side = v3_norm(v3_cross(up, head->along));
	float r = head->radius;
	vec3 at;
	int i;

	for (i = -1; i <= 1; i += 2) {
		at = v3_add(head->centre, v3_scale(head->along, 0.45f * r));
		at = v3_add(at, v3_scale(side, 0.5f * r * (float)i));
		at = v3_add(at, v3_scale(up, 0.74f * r));

		canvas_point(c, at, at.z + 0.35f * r, glyph[eyes], EYE);
	}
}

void
skin_draw(struct canvas *c, const struct body *b, const struct look *look)
{
	struct section between;
	int i, k, last = b->length - 1;

	shape(b, look);
	frame(b->length);

	for (i = 0; i < last; i++) {
		for (k = 0; k < SUBSTEPS; k++) {
			between = blend(&sections[i], &sections[i + 1],
			    (float)k / SUBSTEPS);
			ring(c, &between, between.along, 0, look->glow);
		}
	}
	ring(c, &sections[last], sections[last].along, 0, look->glow);

	cap(c, &sections[0], sections[0].along, look->glow);
	cap(c, &sections[last], v3_scale(sections[last].along, -1), look->glow);
	eyes(c, &sections[0], look->eyes);
}
