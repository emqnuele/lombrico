#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "term.h"

#define ENTER "\x1b[?1049h\x1b[?25l\x1b[?7l\x1b[?1000h\x1b[?1006h\x1b[2J"
#define LEAVE "\x1b[0m\x1b[?1006l\x1b[?1000l\x1b[?7h\x1b[?25h\x1b[?1049l"

static int tty = -1;
static struct termios cooked, raw;

static char input[64];
static size_t input_len;

static volatile sig_atomic_t resized, scrambled, quitting;

void
term_write(const char *bytes, size_t len)
{
	ssize_t n;

	while (len > 0) {
		n = write(tty, bytes, len);
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			return;
		}
		bytes += n;
		len -= (size_t)n;
	}
}

static void
enter(void)
{
	tcsetattr(tty, TCSANOW, &raw);
	term_write(ENTER, sizeof ENTER - 1);
}

static void
leave(void)
{
	term_write(LEAVE, sizeof LEAVE - 1);
	tcsetattr(tty, TCSANOW, &cooked);
}

static void
trap(int sig, void (*handler)(int))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof sa);
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sigaction(sig, &sa, NULL);
}

static void
on_resize(int sig)
{
	(void)sig;
	resized = 1;
	scrambled = 1;
}

static void
on_quit(int sig)
{
	(void)sig;
	quitting = 1;
}

static void
on_stop(int sig)
{
	int saved = errno;
	sigset_t mask;

	leave();
	trap(sig, SIG_DFL);

	sigemptyset(&mask);
	sigaddset(&mask, sig);
	pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
	raise(sig);

	trap(sig, on_stop);
	enter();
	resized = 1;
	scrambled = 1;

	errno = saved;
}

int
term_open(void)
{
	tty = open("/dev/tty", O_RDWR | O_NOCTTY | O_CLOEXEC);
	if (tty < 0) {
		return -1;
	}

	if (tcgetattr(tty, &cooked) < 0) {
		close(tty);
		tty = -1;
		return -1;
	}

	raw = cooked;
	raw.c_lflag &= ~(tcflag_t)(ICANON | ECHO | IEXTEN);
	raw.c_iflag &= ~(tcflag_t)(IXON | ICRNL);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0;

	trap(SIGWINCH, on_resize);
	trap(SIGINT, on_quit);
	trap(SIGTERM, on_quit);
	trap(SIGHUP, on_quit);
	trap(SIGQUIT, on_quit);
	trap(SIGTSTP, on_stop);

	trap(SIGPIPE, SIG_IGN);

	enter();
	scrambled = 1;

	return 0;
}

void
term_close(void)
{
	if (tty < 0) {
		return;
	}

	trap(SIGTSTP, SIG_DFL);
	trap(SIGWINCH, SIG_DFL);
	leave();
	close(tty);
	tty = -1;
}

void
term_size(int *width, int *height)
{
	struct winsize ws;

	if (ioctl(tty, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
		*width = ws.ws_col;
		*height = ws.ws_row;
	} else {
		*width = 80;
		*height = 24;
	}
}

bool
term_resized(void)
{
	if (!resized) {
		return false;
	}
	resized = 0;
	return true;
}

bool
term_scrambled(void)
{
	if (!scrambled) {
		return false;
	}
	scrambled = 0;
	return true;
}

bool
term_quitting(void)
{
	return quitting;
}

bool
term_press(struct press *p)
{
	bool fresh = false, got;
	size_t used;
	ssize_t n;

	for (;;) {
		n = read(tty, input + input_len, sizeof input - input_len);
		if (n > 0) {
			input_len += (size_t)n;
			fresh = true;
		}
		if (input_len == 0) {
			return false;
		}

		used = press_parse(input, input_len, p, &got);
		if (used == 0) {
			if (!fresh || input_len == sizeof input) {
				input_len = 0;
			}
			return false;
		}
		memmove(input, input + used, input_len - used);
		input_len -= used;

		if (got) {
			return true;
		}
	}
}
