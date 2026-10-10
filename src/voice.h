#ifndef LOMBRICO_VOICE_H
#define LOMBRICO_VOICE_H

#include <stdbool.h>

#include "canvas.h"

#define VOICE_LINE 48

enum urgency {
	URGENCY_IDLE,
	URGENCY_NEWS,
	URGENCY_REPLY,
	URGENCY_ALARM,
};

struct voice {
	char         line[VOICE_LINE];
	int          length;
	enum urgency urgency;
	float        age;
	float        life;
	float        quiet;
};

#if defined(__GNUC__)
#define PRINTF_LIKE(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define PRINTF_LIKE(fmt, args)
#endif

bool voice_say(struct voice *, enum urgency, const char *format, ...)
    PRINTF_LIKE(3, 4);
void voice_age(struct voice *, float dt);
void voice_draw(const struct voice *, struct canvas *, vec3 head, float girth);

#endif
