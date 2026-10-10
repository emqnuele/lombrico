#include <stdio.h>
#include <string.h>

#include "rng.h"
#include "words.h"

struct phrases {
	const char *const *lines;
	int                count;
};

#define LINES(...) {                                                    \
	(const char *const []){ __VA_ARGS__ },                          \
	sizeof((const char *const []){ __VA_ARGS__ }) / sizeof(char *), \
}

static const struct phrases book[SAY_OCCASIONS] = {
	[SAY_MORNING] = LINES(
	    "good morning",
	    "morning! fresh soil today",
	    "the early bird gets the worm. not today",
	    "coffee? I'll have some dirt"),
	[SAY_AFTERNOON] = LINES(
	    "good afternoon",
	    "afternoon. the soil is warm",
	    "oh hi. lunch already?"),
	[SAY_EVENING] = LINES(
	    "good evening",
	    "evening. the soil cools down",
	    "long day?"),
	[SAY_NIGHT] = LINES(
	    "it's late, you know",
	    "night owl, huh",
	    "shouldn't we both be asleep?",
	    "a midnight snack?"),

	[SAY_FIRST_FOOD] = LINES(
	    "food!",
	    "oh, text!",
	    "is that for me?"),
	[SAY_SNACK] = LINES(
	    "more text",
	    "oh, a snack",
	    "crunchy",
	    "nom",
	    "delicious logs"),
	[SAY_SPICY] = LINES(
	    "spicy",
	    "ouch, an error",
	    "errors taste like copper",
	    "hot hot hot",
	    "that one bites back"),
	[SAY_WARNING] = LINES(
	    "hm, a warning",
	    "warnings taste a bit sour",
	    "noted. and eaten"),
	[SAY_FLOOD] = LINES(
	    "too much to eat",
	    "slow down",
	    "I can't keep up",
	    "that's a firehose"),
	[SAY_FULL] = LINES(
	    "so full",
	    "I couldn't eat another byte",
	    "food coma incoming"),
	[SAY_BURP] = LINES(
	    "burp",
	    "excuse me",
	    "*burp*",
	    "pardon me"),
	[SAY_ERROR_COUNT] = LINES(
	    "that's {} errors now",
	    "{} errors so far",
	    "{} errors. are you ok?"),
	[SAY_MILESTONE] = LINES(
	    "{} characters eaten",
	    "{} characters! a feast",
	    "{} characters down"),
	[SAY_CLEAN] = LINES(
	    "clean run, nice",
	    "no errors. well done",
	    "all eaten, and not one error"),
	[SAY_DIRTY] = LINES(
	    "{} errors in there",
	    "{} errors. tasty though",
	    "{} errors. maybe fix them?"),

	[SAY_TUNE] = LINES(
	    "♪ oh, music",
	    "I like this one",
	    "♪♪",
	    "this one's a banger",
	    "dance break"),
	[SAY_SLOW_TUNE] = LINES(
	    "a slow dance",
	    "{} bpm, nice and slow",
	    "a slow one. sway with me"),
	[SAY_FAST_TUNE] = LINES(
	    "{} bpm?! too fast",
	    "whoa, fast",
	    "my segments can't keep up"),
	[SAY_TEMPO] = LINES(
	    "{} bpm",
	    "good tempo",
	    "keep it coming"),
	[SAY_SONG_OVER] = LINES(
	    "aw, it stopped",
	    "that was a good one",
	    "encore?"),

	[SAY_NAP] = LINES(
	    "zz",
	    "nap time",
	    "just resting my segments",
	    "wake me if anything happens"),
	[SAY_WAKE] = LINES(
	    "hm?",
	    "I'm up",
	    "what was that",
	    "I wasn't sleeping"),
	[SAY_GRUMPY] = LINES(
	    "five more minutes",
	    "rude",
	    "I was dreaming of compost",
	    "hey, I was sleeping"),

	[SAY_THOUGHT] = LINES(
	    "do you ever think about soil?",
	    "I'm not a snake, by the way",
	    "real worms have five hearts",
	    "bytes are crunchier than bits",
	    "is it a feature or a bug?",
	    "worms can't see. I'm special",
	    "I'm a few thousand lines of c",
	    "rain is my favourite weather",
	    "what's beyond the terminal?",
	    "segments: many. bones: none",
	    "a worm's life is simple",
	    "have you tried turning it off?",
	    "I could use a leaf. press space"),
	[SAY_TAIL] = LINES(
	    "where does it end?",
	    "I'll get it this time",
	    "so close"),
	[SAY_ZOOMIES] = LINES(
	    "zoom",
	    "zoomies!",
	    "gotta go fast"),
	[SAY_STARE] = LINES(
	    "...",
	    "hello there",
	    "I see you",
	    "what are you looking at?"),
	[SAY_STRETCH] = LINES(
	    "stretch",
	    "ahh, that's better",
	    "long worm is long"),

	[SAY_BOOP] = LINES(
	    "boop",
	    "hey, my nose",
	    "that's my face"),
	[SAY_PET] = LINES(
	    "hehe",
	    "that tickles",
	    "♥",
	    "more please",
	    "purr. wait, wrong animal"),
	[SAY_ENOUGH] = LINES(
	    "ok ok, enough",
	    "personal space!",
	    "I'm going over there",
	    "stop it"),
	[SAY_LEAF] = LINES(
	    "yum, a leaf",
	    "thank you",
	    "fresh greens",
	    "my favourite"),
	[SAY_UNKNOWN_KEY] = LINES(
	    "{}? press ? for keys",
	    "I don't know {}. try ?"),
	[SAY_HELP] = LINES(
	    "q quit · i vitals · m mute · space leaf"),
	[SAY_STARTLE] = LINES(
	    "whoa!",
	    "what was that?!",
	    "you scared me",
	    "!!"),

	[SAY_HOT] = LINES(
	    "cpu {}, it's hot down here",
	    "{} cpu? I'm sweating",
	    "cpu {}. it's an oven in here"),
	[SAY_CRAMPED] = LINES(
	    "memory {}, the soil is tight",
	    "{} memory used. crowded",
	    "I can barely move. memory {}"),
	[SAY_RELIEF] = LINES(
	    "ah, room to breathe",
	    "memory is back",
	    "much better"),
	[SAY_COMPILING] = LINES(
	    "who's compiling?",
	    "something woke up the cpu",
	    "busy busy"),
	[SAY_DOWNLOAD] = LINES(
	    "something big is coming in",
	    "downloading the internet?",
	    "{} incoming"),
	[SAY_UPLOAD] = LINES(
	    "sending something big?",
	    "{} going out",
	    "uploading my memories"),
	[SAY_SHRUNK] = LINES(
	    "hey, it's tight in here",
	    "who shrank the soil?",
	    "less room. fine"),
	[SAY_GREW] = LINES(
	    "more room!",
	    "ooh, space",
	    "a bigger garden"),

	[SAY_TOGETHER] = LINES(
	    "{} minutes together",
	    "we've been here {} minutes",
	    "{} minutes. time flies"),
	[SAY_HOUR] = LINES(
	    "an hour together",
	    "one hour already",
	    "an hour. need a break?"),
};

static int last[SAY_OCCASIONS];

int
words_count(enum occasion o)
{
	return book[o].count;
}

const char *
words_line(enum occasion o, int index)
{
	return book[o].lines[index];
}

static const char *
pick(enum occasion o)
{
	int count = book[o].count, i;

	i = (int)(rng_next() % (uint32_t)count);
	if (count > 1 && i == last[o]) {
		i = (i + 1 + (int)(rng_next() % (uint32_t)(count - 1))) % count;
	}
	last[o] = i;
	return book[o].lines[i];
}

bool
words_say(struct voice *v, enum urgency urgency, enum occasion o,
    const char *value)
{
	const char *line = pick(o);
	const char *slot = strstr(line, WORDS_SLOT);

	if (slot == NULL) {
		return voice_say(v, urgency, "%s", line);
	}
	return voice_say(v, urgency, "%.*s%s%s", (int)(slot - line), line,
	    value != NULL ? value : "", slot + strlen(WORDS_SLOT));
}

void
words_rate(char *out, size_t cap, float bytes)
{
	if (bytes >= 1e6f) {
		snprintf(out, cap, "%.1f MB/s", (double)(bytes / 1e6f));
	} else if (bytes >= 1e3f) {
		snprintf(out, cap, "%.0f kB/s", (double)(bytes / 1e3f));
	} else {
		snprintf(out, cap, "%.0f B/s", (double)bytes);
	}
}
