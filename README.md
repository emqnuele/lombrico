# lombrico

I wanted to experiment what i could make in C, i wanted a worm, so I made a cool worm.

lombrico is a worm that lives in your terminal.

it's rendered as a shaded 3D tube in ASCII, but instead of just moving randomly, it actually reacts to what you're doing. If you pipe logs into it, it starts eating them line by line (errors first, obviously). It bounces to whatever music is playing, speeds up when CPU usage spikes, and if you leave it alone long enough, it just curls up and goes to sleep. :)

```
make 2>&1 | lombrico
```

## Build

Plain C17, no dependencies, just whatever the OS already has.

macOS and Linux:

```
make
make install   # optional, puts it in /usr/local/bin so you can just type lombrico
```

Windows 10 or newer, from an MSYS2 MinGW shell:

```
pacman -S mingw-w64-ucrt-x86_64-gcc make
make
```

or cross-compile it from macOS/Linux with zig:

```
make PLATFORM=windows CC="zig cc -target x86_64-windows-gnu"
```

`make test` runs the tests, `make debug` builds `lombrico-debug` with the sanitizers on.

## Usage

```
lombrico                         # just the worm
make 2>&1 | lombrico             # the worm eats your build output
make 2>&1 | lombrico | tee log   # same, but the output still goes through untouched
lombrico --deaf                  # never touches the microphone
```

| you do           | it does                                  |
| ---------------- | ---------------------------------------- |
| click its head   | boop                                     |
| click its body   | gets petted (until it's had enough)      |
| click elsewhere  | a leaf falls there and it goes to eat it |
| `space`          | drops a leaf somewhere random            |
| `?`              | shows the keys                           |
| `i`              | shows/hides the stats line               |
| `m`              | stops/starts listening                   |
| `q`              | quits                                    |

A few things worth knowing:

- While it runs, the mouse is the worm's. Hold shift if you want to select text.
- On macOS the terminal asks for the microphone the first time. If you say no it works fine, it just never dances.
- Pipe stderr in too (`2>&1`). Anything that gets printed straight to the terminal ends up drawn over the worm.

## What it does

It's always in one of four moods, and it checks them in this order: **eat** if there's something to eat, **dance** if it hears music, **sleep** if nothing happened for 30 seconds (15 at night, it has a bedtime), otherwise it just **wanders** around.

### Eating

Whatever you pipe in shows up at the bottom and scrolls up like a normal log. The worm goes for the closest characters, but it really likes errors and will go out of its way for one. Every character it touches gets eaten and falls off as a crumb, and you can see each bite go down its body as a little bulge.

Lines get a flavour:

- **plain**: grey, nothing special
- **warn**: anything with `warn`, amber, a bit sour apparently
- **error**: `error`, `fail`, `fatal`, `panic`, `exception`, red. Spicy. It turns gold inside and runs hot for a bit

If stdout isn't the terminal, every byte goes through exactly as it came in, so you can stick it in the middle of a pipeline and nothing breaks.

If you feed it something huge (`log stream --level debug` on macOS, for example) it can't keep up, obviously. It only draws 15 new lines a second, everything else still goes through to stdout, and it tells you to slow down. It also doesn't bloat into a giant blob anymore, that was a fun bug.

It keeps track of what it eats. Eat too much and it gets full, slows down, and burps. Yes it burps. Every 10 errors it tells you the count, it celebrates 1k, 10k, 100k characters, and when the input ends it gives you a verdict: clean run, or how many errors you have to go fix.

### Dancing

It listens to the mic and dances when it hears beats that keep time. That's how it tells music apart from you talking or typing: music has a steady gap between beats, everything else doesn't. It also figures out the bpm and dances at that speed, and it has opinions about it (under 95 is a slow dance, over 140 is too fast for a worm).

It listens a bit above the bass too, because laptop speakers basically have no bass and otherwise it would never hear anything.

Eating comes first, but it never fully ignores music: even with its mouth full it turns pink and throws a ♪ on every beat.

### Sleeping

It slows down while still turning, so it ends up curled in a spiral. Eyes close, it breathes, `z`s float up. Anything wakes it up. If you wake it up by clicking on it, it's grumpy about it, thats fair IMO.

### Being bored

If nothing's happening for a while it gets bored and does something dumb for a few seconds: chases its own tail, does zoomies across the screen, stops and stares at you, or stretches out longer. Every now and then it also just says something, your cpu and memory or some random thought it had.

### Clicks and keys

Boop it on the head and it blinks. Click the body to pet it and hearts come out. Pet it too much too fast and it tells you to stop and crawls away. Click anywhere else and a leaf falls there, it goes and eats it and says thanks. `space` drops one somewhere random. Press a key it doesn't know and it asks you what that was.

### Getting scared

A sudden loud noise when there's no music, like a clap or a door slamming, makes it jump. It bolts, flashes, and a `!` pops up over its head.

### The machine

- **cpu** heats it up: red goes orange, it gets faster and twitchier. Over 85% it complains.
- **memory** makes it longer. Over 85% it says it's getting tight in there.
- **network** traffic sends ripples along its body, in for downloads, out for uploads. Big ones get a comment.
- if the cpu suddenly jumps from idle to busy it asks who's compiling
- resize the window and it notices

It also says hi based on the time of day, and keeps track of how long you've been hanging out together.

### Talking

It talks in a little bubble over its head, one letter at a time. Every situation has a few different lines and it never says the same one twice in a row. Answering you always comes first, and random chatter waits for a few seconds of quiet, so it doesn't talk your ear off.

All the lines are in `src/words.c` if you want to change what it says.

## How it's made

The body is a chain of segments where only the head steers and every other segment just follows the one in front. That's all it takes to get the snake-like movement. The tube is lit from the top left and every cell picks a character from ` .:-=+*#%@` based on how bright it is, same trick as `donut.c`.

It runs at 30fps, only redraws the cells that changed, and puts your terminal back exactly how it was when you quit (ctrl-c and ctrl-z included).

| file          | what's in it                                                   |
| ------------- | -------------------------------------------------------------- |
| `main.c`      | the main loop: read input, think, move, draw                   |
| `mind.c`      | moods, whims, reactions to clicks and keys                     |
| `notice.c`    | the stuff it comments on: cpu, memory, window size, time, etc. |
| `words.c`     | every line it can say                                          |
| `body.c`      | the spine, how the head steers, waves running along the body   |
| `skin.c`      | turns the spine into a lit 3D tube made of characters          |
| `food.c`      | reads stdin, cleans it up, figures out what's an error         |
| `ear.c`       | the microphone side: FFT, beats, tempo, "is this music?"       |
| `leaves.c`    | the leaves you drop                                            |
| `press.c`     | parses keys and mouse clicks from the terminal                 |
| `voice.c`     | the speech bubble                                              |
| `motes.c`     | crumbs, notes, hearts, z's                                     |
| `canvas.c`    | a grid of cells with depth                                     |
| `paint.c`     | sends only the cells that changed to the terminal              |

Everything OS-specific is split into one file per OS, the rest of the code doesn't care where it runs:

| what              | macOS              | Linux                    | Windows             |
| ----------------- | ------------------ | ------------------------ | ------------------- |
| terminal          | `term_posix.c`     | `term_posix.c`           | `term_windows.c`    |
| stdin/stdout      | `stream_posix.c`   | `stream_posix.c`         | `stream_windows.c`  |
| time              | `tick_posix.c`     | `tick_posix.c`           | `tick_windows.c`    |
| cpu, memory, net  | `soil_darwin.c`    | `soil_linux.c`           | `soil_windows.c`    |
| microphone        | `capture_darwin.c` | `capture_linux.c` (ALSA) | `capture_windows.c` |

Windows has no load average so the stats line skips it there. On Linux without ALSA it runs fine, it just never dances.
