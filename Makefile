PROG	 = lombrico
PREFIX	?= /usr/local

ifeq ($(OS),Windows_NT)
PLATFORM ?= windows
else ifeq ($(shell uname -s),Darwin)
PLATFORM ?= darwin
else
PLATFORM ?= linux
endif

CORE	 = body canvas ear food hud leaves main mind motes notice paint press \
	   rng skin utf8 voice words
POSIX	 = term_posix stream_posix tick_posix

ifeq ($(PLATFORM),darwin)
UNITS	 = $(CORE) $(POSIX) soil_darwin capture_darwin
CPPFLAGS += -D_DARWIN_C_SOURCE
LDLIBS	 = -lm -framework AudioToolbox -framework CoreFoundation
else ifeq ($(PLATFORM),linux)
UNITS	 = $(CORE) $(POSIX) soil_linux capture_linux
CPPFLAGS += -D_DEFAULT_SOURCE
THREADS	 = -pthread
LDLIBS	 = -lm -ldl
else ifeq ($(PLATFORM),windows)
UNITS	 = $(CORE) term_windows stream_windows tick_windows soil_windows \
	   capture_windows
CPPFLAGS += -U_WIN32_WINNT -D_WIN32_WINNT=0x0A00 -DWIN32_LEAN_AND_MEAN
LDLIBS	 = -lwinmm -liphlpapi
EXE	 = .exe
else
$(error unknown platform $(PLATFORM), use darwin, linux or windows)
endif

CC	?= cc
CFLAGS	?= -O2
WARN	 = -Wall -Wextra -Wpedantic -Wshadow -Wvla -Wdouble-promotion \
	   -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS += -MMD -MP

BUILD	?= build/$(PLATFORM)
BIN	?= $(PROG)$(EXE)
OBJ	 = $(UNITS:%=$(BUILD)/%.o)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(THREADS) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) -std=c17 $(WARN) $(WERROR) $(THREADS) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $@

TESTS	 = $(BUILD)/ear_test$(EXE) $(BUILD)/food_test$(EXE) \
	   $(BUILD)/press_test$(EXE) $(BUILD)/words_test$(EXE)

test: $(TESTS)
	for t in $(TESTS); do $$t || exit 1; done

$(BUILD)/ear_test$(EXE): tests/ear_test.c tests/capture_fake.c src/ear.c src/rng.c | $(BUILD)
	$(CC) -std=c17 $(WARN) $(WERROR) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lm

$(BUILD)/food_test$(EXE): tests/food_test.c tests/stream_fake.c src/food.c src/utf8.c src/canvas.c | $(BUILD)
	$(CC) -std=c17 $(WARN) $(WERROR) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lm

$(BUILD)/press_test$(EXE): tests/press_test.c src/press.c src/utf8.c | $(BUILD)
	$(CC) -std=c17 $(WARN) $(WERROR) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(BUILD)/words_test$(EXE): tests/words_test.c src/words.c src/voice.c src/rng.c src/utf8.c src/canvas.c | $(BUILD)
	$(CC) -std=c17 $(WARN) $(WERROR) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lm

debug:
	$(MAKE) BUILD=build/$(PLATFORM)-debug BIN=$(PROG)-debug$(EXE) WERROR=-Werror \
		CFLAGS="-O0 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined" \
		LDFLAGS="-fsanitize=address,undefined"

install: $(BIN)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)

clean:
	rm -rf build $(PROG) $(PROG)-debug $(PROG).exe $(PROG)-debug.exe

.PHONY: all test debug install uninstall clean

-include $(OBJ:.o=.d)
