#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include "stream.h"

static bool
redirected(int fd)
{
	return fcntl(fd, F_GETFD) != -1 && !isatty(fd);
}

bool
stream_fed(void)
{
	return redirected(STDIN_FILENO);
}

bool
stream_piped_out(void)
{
	return redirected(STDOUT_FILENO);
}

long
stream_read(char *bytes, size_t cap)
{
	struct pollfd in = { .fd = STDIN_FILENO, .events = POLLIN };
	ssize_t n;

	if (poll(&in, 1, 0) <= 0) {
		return 0;
	}

	n = read(STDIN_FILENO, bytes, cap);
	if (n < 0) {
		return errno == EINTR || errno == EAGAIN ? 0 : STREAM_ENDED;
	}
	return n == 0 ? STREAM_ENDED : (long)n;
}

bool
stream_pass(const char *bytes, size_t len)
{
	ssize_t n;

	while (len > 0) {
		n = write(STDOUT_FILENO, bytes, len);
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			return false;
		}
		bytes += n;
		len -= (size_t)n;
	}
	return true;
}
