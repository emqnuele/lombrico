#ifndef LOMBRICO_EAR_H
#define LOMBRICO_EAR_H

#include <stdbool.h>

struct hearing {
	float level;
	float bass;
	float mid;
	float treble;
	float beat;
	bool  struck;
	float music;
	float tempo;
	bool  jolted;
};

int  ear_open(void);
void ear_listen(struct hearing *, float dt);
void ear_close(void);

#endif
