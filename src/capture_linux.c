#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>

#include "capture.h"

#define FRAMES  1024
#define RATE    44100
#define LATENCY 100000

#define SND_PCM_STREAM_CAPTURE        1
#define SND_PCM_FORMAT_FLOAT_LE       14
#define SND_PCM_ACCESS_RW_INTERLEAVED 3

typedef struct snd_pcm snd_pcm_t;

static struct {
	int  (*open)(snd_pcm_t **, const char *, int, int);
	int  (*set_params)(snd_pcm_t *, int, int, unsigned, unsigned, int,
	    unsigned);
	long (*readi)(snd_pcm_t *, void *, unsigned long);
	int  (*recover)(snd_pcm_t *, int, int);
	int  (*close)(snd_pcm_t *);
} alsa;

static void *library;
static snd_pcm_t *pcm;
static pthread_t thread;
static atomic_bool running;

static bool
resolve(const char *name, void *slot, size_t size)
{
	void *symbol = dlsym(library, name);

	if (symbol == NULL) {
		return false;
	}
	memcpy(slot, &symbol, size);
	return true;
}

#define RESOLVE(call) resolve("snd_pcm_" #call, &alsa.call, sizeof alsa.call)

static void *
listen_loop(void *ring)
{
	float frames[FRAMES];
	long got;

	while (atomic_load(&running)) {
		got = alsa.readi(pcm, frames, FRAMES);
		if (got < 0) {
			if (alsa.recover(pcm, (int)got, 1) < 0) {
				break;
			}
			continue;
		}
		ring_push(ring, frames, (uint32_t)got);
	}
	return NULL;
}

static void
unload(void)
{
	if (pcm != NULL) {
		alsa.close(pcm);
		pcm = NULL;
	}
	if (library != NULL) {
		dlclose(library);
		library = NULL;
	}
}

int
capture_start(struct ring *ring, float *rate)
{
	library = dlopen("libasound.so.2", RTLD_NOW | RTLD_LOCAL);
	if (library == NULL) {
		return -1;
	}

	if (!RESOLVE(open) || !RESOLVE(set_params) || !RESOLVE(readi) ||
	    !RESOLVE(recover) || !RESOLVE(close)) {
		unload();
		return -1;
	}

	if (alsa.open(&pcm, "default", SND_PCM_STREAM_CAPTURE, 0) < 0) {
		pcm = NULL;
		unload();
		return -1;
	}
	if (alsa.set_params(pcm, SND_PCM_FORMAT_FLOAT_LE,
	    SND_PCM_ACCESS_RW_INTERLEAVED, 1, RATE, 1, LATENCY) < 0) {
		unload();
		return -1;
	}

	atomic_store(&running, true);
	if (pthread_create(&thread, NULL, listen_loop, ring) != 0) {
		atomic_store(&running, false);
		unload();
		return -1;
	}

	*rate = RATE;
	return 0;
}

void
capture_stop(void)
{
	if (!atomic_load(&running)) {
		return;
	}
	atomic_store(&running, false);
	pthread_join(thread, NULL);
	unload();
}
