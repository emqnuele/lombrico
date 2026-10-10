#include <stdio.h>
#include <string.h>

#include "../src/food.h"
#include "stream_fake.h"

static struct food food;
static int failures;

static void
expect(bool ok, const char *what)
{
	printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
	failures += !ok;
}

static const char *
line(int k)
{
	static char text[FOOD_WIDTH + 1];
	const struct line *l;
	int x;

	l = &food.lines[(food.newest - k + FOOD_LINES) % FOOD_LINES];

	for (x = 0; x < l->width; x++) {
		text[x] = l->glyphs[x] < 0x80 ? (char)l->glyphs[x] : '#';
	}
	text[l->width] = '\0';
	return text;
}

static const char *
flood(void)
{
	static char text[1000 * 6 + 1];
	int i;

	for (i = 0; i < 1000; i++) {
		memcpy(text + i * 6, "flood\n", 6);
	}
	return text;
}

static void
eat(const char *bytes)
{
	food_open(&food);
	stream_give(bytes);
	while (food.open) {
		food_read(&food, 0);
	}
}

int
main(void)
{
	struct meal meal;
	vec3 at;

	eat("\x1b[32m[build]\x1b[0m ok\n");
	expect(!strcmp(line(0), "[build] ok"), "colours are stripped");

	eat("\x1b]0;title\a" "after osc\n");
	expect(!strcmp(line(0), "after osc"), "osc titles are stripped");

	eat("a\tb\n");
	expect(!strcmp(line(0), "a   b"), "tabs expand to the next stop");

	eat("one\r\ntwo\r\n");
	expect(food.count == 2 && !strcmp(line(0), "two"), "crlf is one line ending");

	eat("10%\r50%\r100%\n");
	expect(food.count == 1 && !strcmp(line(0), "100%"), "carriage return rewinds");

	eat("   \n\n\t\n");
	expect(food.count == 0, "blank lines are not food");

	eat("no newline at the end");
	expect(food.count == 1 && !strcmp(line(0), "no newline at the end"),
	    "the last line is served at the end of input");

	eat("caf\xc3\xa9 \xe4\xb8\xad" "\n");
	expect(!strcmp(line(0), "caf# ?"), "wide glyphs become a question mark");

	eat("\xc0\xaf\n");
	expect(food.count == 1 && line(0)[0] == '?', "overlong utf-8 is rejected");

	eat("all good\nWARNING: deprecated\nError: boom\nthe build FAILED\n");
	expect(food.lines[food.newest].flavour == FLAVOUR_ERROR, "failed is an error");
	expect(food.lines[(food.newest + FOOD_LINES - 1) % FOOD_LINES].flavour ==
	    FLAVOUR_ERROR, "errors are found in any case");
	expect(food.lines[(food.newest + FOOD_LINES - 2) % FOOD_LINES].flavour ==
	    FLAVOUR_WARN, "warnings are found");
	expect(food.lines[(food.newest + FOOD_LINES - 3) % FOOD_LINES].flavour ==
	    FLAVOUR_PLAIN, "plain lines stay plain");

	eat("hello\nan error\n");
	food_place(&food, 80, 10);
	expect(food_nearest(&food, v3(0, 20, 0), &at) && at.y > 20,
	    "the worm prefers the error even when it is further");

	food_eat(&food, v3(3.5f, 21, 0), 1.2f, &meal);
	expect(meal.count == 2 && meal.errors == 2, "a bite takes what is in reach");

	food_eat(&food, v3(4, 21, 0), 3.9f, &meal);
	expect(meal.lines == 1 && food.swallowed == 1, "a finished line is swallowed");

	food_place(&food, 3, 10);
	food_eat(&food, v3(4, 19, 0), 2, &meal);
	expect(meal.count == 0, "off-screen text cannot be eaten");

	eat(flood());
	expect(food.count == 40 && food.skipped == 960,
	    "a flood reaches the screen only in part");

	return failures != 0;
}
