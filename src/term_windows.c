#include <windows.h>
#include <mmsystem.h>

#include "term.h"

#define ENTER "\x1b[?1049h\x1b[?25l\x1b[?7l\x1b[2J"
#define LEAVE "\x1b[0m\x1b[?7h\x1b[?25h\x1b[?1049l"

static HANDLE in = INVALID_HANDLE_VALUE;
static HANDLE out = INVALID_HANDLE_VALUE;
static DWORD in_mode, out_mode;
static UINT code_page;
static int known_width, known_height;

static volatile LONG quitting, scrambled;

static BOOL WINAPI
on_control(DWORD kind)
{
	(void)kind;
	InterlockedExchange(&quitting, 1);
	return TRUE;
}

void
term_write(const char *bytes, size_t len)
{
	DWORD done;

	while (len > 0) {
		if (!WriteFile(out, bytes, (DWORD)len, &done, NULL)) {
			return;
		}
		bytes += done;
		len -= done;
	}
}

static void
release(void)
{
	if (in != INVALID_HANDLE_VALUE) {
		CloseHandle(in);
		in = INVALID_HANDLE_VALUE;
	}
	if (out != INVALID_HANDLE_VALUE) {
		CloseHandle(out);
		out = INVALID_HANDLE_VALUE;
	}
}

int
term_open(void)
{
	in = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE,
	    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	out = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
	    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (in == INVALID_HANDLE_VALUE || out == INVALID_HANDLE_VALUE ||
	    !GetConsoleMode(in, &in_mode) || !GetConsoleMode(out, &out_mode)) {
		release();
		return -1;
	}

	if (!SetConsoleMode(out, out_mode | ENABLE_PROCESSED_OUTPUT |
	    ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN)) {
		release();
		return -1;
	}
	SetConsoleMode(in, ENABLE_PROCESSED_INPUT | ENABLE_WINDOW_INPUT |
	    ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);

	code_page = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCtrlHandler(on_control, TRUE);

	timeBeginPeriod(1);

	term_write(ENTER, sizeof ENTER - 1);
	term_size(&known_width, &known_height);
	scrambled = 1;

	return 0;
}

void
term_close(void)
{
	if (out == INVALID_HANDLE_VALUE) {
		return;
	}

	term_write(LEAVE, sizeof LEAVE - 1);
	SetConsoleMode(in, in_mode);
	SetConsoleMode(out, out_mode);
	SetConsoleOutputCP(code_page);
	SetConsoleCtrlHandler(on_control, FALSE);
	timeEndPeriod(1);
	release();
}

void
term_size(int *width, int *height)
{
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (GetConsoleScreenBufferInfo(out, &info)) {
		*width = info.srWindow.Right - info.srWindow.Left + 1;
		*height = info.srWindow.Bottom - info.srWindow.Top + 1;
	} else {
		*width = 80;
		*height = 24;
	}
}

bool
term_resized(void)
{
	int width, height;

	term_size(&width, &height);
	if (width == known_width && height == known_height) {
		return false;
	}
	known_width = width;
	known_height = height;
	scrambled = 1;
	return true;
}

bool
term_scrambled(void)
{
	return InterlockedExchange(&scrambled, 0) != 0;
}

bool
term_quitting(void)
{
	return quitting != 0;
}

static bool
key_of(const KEY_EVENT_RECORD *key, struct press *p)
{
	if (!key->bKeyDown || key->uChar.UnicodeChar == 0) {
		return false;
	}
	*p = (struct press){ .kind = PRESS_KEY, .key = key->uChar.UnicodeChar };
	return true;
}

static bool
click_of(const MOUSE_EVENT_RECORD *mouse, struct press *p)
{
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (mouse->dwEventFlags != 0 ||
	    !(mouse->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) ||
	    !GetConsoleScreenBufferInfo(out, &info)) {
		return false;
	}
	*p = (struct press){
		.kind = PRESS_CLICK,
		.x = mouse->dwMousePosition.X - info.srWindow.Left,
		.y = mouse->dwMousePosition.Y - info.srWindow.Top,
	};
	return true;
}

bool
term_press(struct press *p)
{
	INPUT_RECORD record;
	DWORD waiting, got;

	while (GetNumberOfConsoleInputEvents(in, &waiting) && waiting > 0) {
		if (!ReadConsoleInputW(in, &record, 1, &got) || got == 0) {
			break;
		}
		if (record.EventType == KEY_EVENT &&
		    key_of(&record.Event.KeyEvent, p)) {
			return true;
		}
		if (record.EventType == MOUSE_EVENT &&
		    click_of(&record.Event.MouseEvent, p)) {
			return true;
		}
	}
	return false;
}
