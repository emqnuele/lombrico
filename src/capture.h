#ifndef LOMBRICO_CAPTURE_H
#define LOMBRICO_CAPTURE_H

#include "ring.h"

int  capture_start(struct ring *, float *rate);
void capture_stop(void);

#endif
