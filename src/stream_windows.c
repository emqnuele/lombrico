#include <windows.h>

#include "stream.h"

static bool
redirected(DWORD which)
{
	HANDLE handle = GetStdHandle(which);
	DWORD type;

	if (handle == NULL || handle == INVALID_HANDLE_VALUE) {
		return false;
	}
	type = GetFileType(handle);
	return type == FILE_TYPE_PIPE || type == FILE_TYPE_DISK;
}

bool
stream_fed(void)
{
	return redirected(STD_INPUT_HANDLE);
}

bool
stream_piped_out(void)
{
	return redirected(STD_OUTPUT_HANDLE);
}

long
stream_read(char *bytes, size_t cap)
{
	HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
	DWORD waiting, got;

	if (GetFileType(in) == FILE_TYPE_PIPE) {
		if (!PeekNamedPipe(in, NULL, 0, NULL, &waiting, NULL)) {
			return STREAM_ENDED;
		}
		if (waiting == 0) {
			return 0;
		}
		if (cap > waiting) {
			cap = waiting;
		}
	}

	if (!ReadFile(in, bytes, (DWORD)cap, &got, NULL) || got == 0) {
		return STREAM_ENDED;
	}
	return (long)got;
}

bool
stream_pass(const char *bytes, size_t len)
{
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD done;

	while (len > 0) {
		if (!WriteFile(out, bytes, (DWORD)len, &done, NULL)) {
			return false;
		}
		bytes += done;
		len -= done;
	}
	return true;
}
