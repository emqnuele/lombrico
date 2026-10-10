#include <windows.h>
#include <mmsystem.h>

#include "capture.h"

#define BUFFERS 4
#define FRAMES  1024
#define RATE    44100

static HWAVEIN wave;
static WAVEHDR headers[BUFFERS];
static short pcm[BUFFERS][FRAMES];
static HANDLE ready, thread;
static volatile LONG running;

static DWORD WINAPI
listen_loop(LPVOID ring)
{
	float frames[FRAMES];
	DWORD count, i;
	int b;

	while (running) {
		WaitForSingleObject(ready, 100);
		for (b = 0; b < BUFFERS && running; b++) {
			if (!(headers[b].dwFlags & WHDR_DONE)) {
				continue;
			}
			count = headers[b].dwBytesRecorded / sizeof(short);
			for (i = 0; i < count; i++) {
				frames[i] = (float)pcm[b][i] / 32768.0f;
			}
			ring_push(ring, frames, count);
			waveInAddBuffer(wave, &headers[b], sizeof headers[b]);
		}
	}
	return 0;
}

static void
close_wave(void)
{
	int b;

	waveInReset(wave);
	for (b = 0; b < BUFFERS; b++) {
		waveInUnprepareHeader(wave, &headers[b], sizeof headers[b]);
	}
	waveInClose(wave);
	wave = NULL;
}

int
capture_start(struct ring *ring, float *rate)
{
	WAVEFORMATEX format = {
		.wFormatTag = WAVE_FORMAT_PCM,
		.nChannels = 1,
		.nSamplesPerSec = RATE,
		.nAvgBytesPerSec = RATE * sizeof(short),
		.nBlockAlign = sizeof(short),
		.wBitsPerSample = 16,
	};
	int b;

	ready = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (ready == NULL) {
		return -1;
	}
	if (waveInOpen(&wave, WAVE_MAPPER, &format, (DWORD_PTR)ready, 0,
	    CALLBACK_EVENT) != MMSYSERR_NOERROR) {
		CloseHandle(ready);
		return -1;
	}

	for (b = 0; b < BUFFERS; b++) {
		headers[b] = (WAVEHDR){
			.lpData = (LPSTR)pcm[b],
			.dwBufferLength = sizeof pcm[b],
		};
		waveInPrepareHeader(wave, &headers[b], sizeof headers[b]);
		waveInAddBuffer(wave, &headers[b], sizeof headers[b]);
	}

	running = 1;
	thread = CreateThread(NULL, 0, listen_loop, ring, 0, NULL);
	if (thread == NULL || waveInStart(wave) != MMSYSERR_NOERROR) {
		capture_stop();
		return -1;
	}

	*rate = RATE;
	return 0;
}

void
capture_stop(void)
{
	if (wave == NULL) {
		return;
	}

	InterlockedExchange(&running, 0);
	if (thread != NULL) {
		SetEvent(ready);
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
		thread = NULL;
	}
	close_wave();
	CloseHandle(ready);
}
