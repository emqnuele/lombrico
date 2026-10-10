#include <stdio.h>
#include <string.h>

#include "../src/rng.h"
#include "../src/words.h"

static int failures;

static void
expect(bool ok, const char *what)
{
	printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
	failures += !ok;
}

static int
glyphs(const char *text)
{
	int count = 0;

	for (; *text != '\0'; text++) {
		count += ((unsigned char)*text & 0xc0) != 0x80;
	}
	return count;
}

int
main(void)
{
	const char *line, *slot;
	bool spoken = true, fits = true, single = true, fresh = true;
	char previous[VOICE_LINE];
	struct voice v;
	int o, i, fill;

	rng_seed(1);

	for (o = 0; o < SAY_OCCASIONS; o++) {
		spoken &= words_count(o) > 0;
		for (i = 0; i < words_count(o); i++) {
			line = words_line(o, i);
			slot = strstr(line, WORDS_SLOT);
			single &= slot == NULL || strstr(slot + 1, WORDS_SLOT) == NULL;

			fill = slot != NULL ? 10 - (int)strlen(WORDS_SLOT) : 0;
			fits &= glyphs(line) + fill <= 40 &&
			    strlen(line) + (size_t)fill < VOICE_LINE;
		}
	}
	expect(spoken, "every occasion has something to say");
	expect(fits, "every line fits a bubble on an 80 column terminal");
	expect(single, "a line takes at most one value");

	for (o = 0; o < SAY_OCCASIONS; o++) {
		if (words_count(o) < 2) {
			continue;
		}
		previous[0] = '\0';
		for (i = 0; i < 50; i++) {
			memset(&v, 0, sizeof v);
			words_say(&v, URGENCY_NEWS, o, "7");
			fresh &= strcmp(v.line, previous) != 0;
			strcpy(previous, v.line);
		}
	}
	expect(fresh, "no line comes twice in a row");

	memset(&v, 0, sizeof v);
	words_say(&v, URGENCY_NEWS, SAY_HOT, "97%");
	expect(strstr(v.line, "97%") != NULL && strstr(v.line, "{}") == NULL,
	    "the value takes the place of the slot");

	memset(&v, 0, sizeof v);
	expect(!words_say(&v, URGENCY_IDLE, SAY_THOUGHT, NULL),
	    "idle talk waits for a pause");
	v.quiet = 10;
	expect(words_say(&v, URGENCY_IDLE, SAY_THOUGHT, NULL),
	    "and speaks after one");

	memset(&v, 0, sizeof v);
	words_say(&v, URGENCY_NEWS, SAY_SNACK, NULL);
	expect(words_say(&v, URGENCY_REPLY, SAY_BOOP, NULL),
	    "a reply interrupts the news");
	expect(words_say(&v, URGENCY_REPLY, SAY_PET, NULL),
	    "and a new reply replaces the last one");
	expect(!words_say(&v, URGENCY_NEWS, SAY_SNACK, NULL),
	    "but news waits for the reply to land");

	return failures != 0;
}
