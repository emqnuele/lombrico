#include <windows.h>

#include "tick.h"

double
tick_now(void)
{
	static LARGE_INTEGER frequency;
	LARGE_INTEGER count;

	if (frequency.QuadPart == 0) {
		QueryPerformanceFrequency(&frequency);
	}
	QueryPerformanceCounter(&count);
	return (double)count.QuadPart / (double)frequency.QuadPart;
}

void
tick_sleep(double seconds)
{
	if (seconds > 0) {
		Sleep((DWORD)(seconds * 1000));
	}
}
