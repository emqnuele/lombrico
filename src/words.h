#ifndef LOMBRICO_WORDS_H
#define LOMBRICO_WORDS_H

#include <stddef.h>

#include "voice.h"

enum occasion {
	SAY_MORNING,
	SAY_AFTERNOON,
	SAY_EVENING,
	SAY_NIGHT,

	SAY_FIRST_FOOD,
	SAY_SNACK,
	SAY_SPICY,
	SAY_WARNING,
	SAY_FLOOD,
	SAY_FULL,
	SAY_BURP,
	SAY_ERROR_COUNT,
	SAY_MILESTONE,
	SAY_CLEAN,
	SAY_DIRTY,

	SAY_TUNE,
	SAY_SLOW_TUNE,
	SAY_FAST_TUNE,
	SAY_TEMPO,
	SAY_SONG_OVER,

	SAY_NAP,
	SAY_WAKE,
	SAY_GRUMPY,

	SAY_THOUGHT,
	SAY_TAIL,
	SAY_ZOOMIES,
	SAY_STARE,
	SAY_STRETCH,

	SAY_BOOP,
	SAY_PET,
	SAY_ENOUGH,
	SAY_LEAF,
	SAY_UNKNOWN_KEY,
	SAY_HELP,
	SAY_STARTLE,

	SAY_HOT,
	SAY_CRAMPED,
	SAY_RELIEF,
	SAY_COMPILING,
	SAY_DOWNLOAD,
	SAY_UPLOAD,
	SAY_SHRUNK,
	SAY_GREW,

	SAY_TOGETHER,
	SAY_HOUR,

	SAY_OCCASIONS,
};

#define WORDS_SLOT "{}"

int         words_count(enum occasion);
const char *words_line(enum occasion, int index);
bool        words_say(struct voice *, enum urgency, enum occasion,
    const char *value);
void        words_rate(char *out, size_t cap, float bytes);

#endif
