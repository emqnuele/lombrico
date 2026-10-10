#include <stdio.h>
#include <string.h>

#include "../src/press.h"

static int failures;

static void
expect(bool ok, const char *what)
{
	printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
	failures += !ok;
}

static int
parse(const char *bytes, size_t len, struct press *out, int cap, size_t *left)
{
	struct press p;
	size_t used;
	bool got;
	int count = 0;

	while (len > 0 && (used = press_parse(bytes, len, &p, &got)) > 0) {
		if (got && count < cap) {
			out[count++] = p;
		}
		bytes += used;
		len -= used;
	}
	*left = len;
	return count;
}

#define PARSE(text, out, left) parse(text, sizeof(text) - 1, out, 8, left)

int
main(void)
{
	struct press p[8];
	size_t left;
	int n;

	n = PARSE("q", p, &left);
	expect(n == 1 && p[0].kind == PRESS_KEY && p[0].key == 'q', "a key");

	n = PARSE("\xc3\xa9", p, &left);
	expect(n == 1 && p[0].key == 0xe9, "a multibyte key");

	n = PARSE("\xc3", p, &left);
	expect(n == 0 && left == 1, "half a multibyte key waits");

	n = PARSE("\x1b[<0;10;5M", p, &left);
	expect(n == 1 && p[0].kind == PRESS_CLICK && p[0].x == 9 && p[0].y == 4,
	    "a left click, at zero based cells");

	n = PARSE("\x1b[<0;10;5m", p, &left);
	expect(n == 0 && left == 0, "a release is swallowed");

	n = PARSE("\x1b[<64;3;3M\x1b[<32;3;3M\x1b[<2;3;3M", p, &left);
	expect(n == 0 && left == 0, "wheel, drag and right click are swallowed");

	n = PARSE("\x1b[<0;10", p, &left);
	expect(n == 0 && left == 7, "half a click waits");

	n = PARSE("\x1b[A\x1bOPz", p, &left);
	expect(n == 1 && p[0].key == 'z', "arrows and function keys are skipped");

	n = PARSE("\x1b", p, &left);
	expect(n == 1 && p[0].key == KEY_ESCAPE, "escape alone is a key");

	n = PARSE("\x1b[", p, &left);
	expect(n == 0 && left == 2, "an unfinished sequence waits");

	n = PARSE("\x1bxq", p, &left);
	expect(n == 1 && p[0].key == 'q', "alt and a key is skipped");

	n = PARSE("\x1b[<0;5M" "k", p, &left);
	expect(n == 1 && p[0].key == 'k', "a malformed click is skipped");

	n = PARSE("\x1b[<0;99999999;1M", p, &left);
	expect(n == 1 && p[0].x > 0, "huge coordinates do not overflow");

	return failures != 0;
}
