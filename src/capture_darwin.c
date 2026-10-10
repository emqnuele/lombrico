#include <AudioToolbox/AudioToolbox.h>

#include "capture.h"

#define BUFFERS 3
#define FRAMES  1024
#define RATE    44100.0

static AudioQueueRef queue;

static void
heard(void *ring, AudioQueueRef q, AudioQueueBufferRef buffer,
    const AudioTimeStamp *when, UInt32 packets,
    const AudioStreamPacketDescription *packet)
{
	(void)when;
	(void)packets;
	(void)packet;

	ring_push(ring, buffer->mAudioData,
	    buffer->mAudioDataByteSize / sizeof(float));
	AudioQueueEnqueueBuffer(q, buffer, 0, NULL);
}

int
capture_start(struct ring *ring, float *rate)
{
	AudioStreamBasicDescription format = {
		.mSampleRate = RATE,
		.mFormatID = kAudioFormatLinearPCM,
		.mFormatFlags = kLinearPCMFormatFlagIsFloat |
		    kLinearPCMFormatFlagIsPacked,
		.mBytesPerPacket = sizeof(float),
		.mFramesPerPacket = 1,
		.mBytesPerFrame = sizeof(float),
		.mChannelsPerFrame = 1,
		.mBitsPerChannel = 32,
	};
	AudioQueueBufferRef buffer;
	int i;

	if (AudioQueueNewInput(&format, heard, ring, NULL, NULL, 0,
	    &queue) != noErr) {
		return -1;
	}

	for (i = 0; i < BUFFERS; i++) {
		if (AudioQueueAllocateBuffer(queue, FRAMES * sizeof(float),
		    &buffer) != noErr ||
		    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL) != noErr) {
			capture_stop();
			return -1;
		}
	}

	if (AudioQueueStart(queue, NULL) != noErr) {
		capture_stop();
		return -1;
	}

	*rate = (float)RATE;
	return 0;
}

void
capture_stop(void)
{
	if (queue == NULL) {
		return;
	}
	AudioQueueStop(queue, true);
	AudioQueueDispose(queue, true);
	queue = NULL;
}
