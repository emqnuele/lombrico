#ifndef LOMBRICO_SOIL_H
#define LOMBRICO_SOIL_H

struct soil {
	float cpu;
	float memory;
	float load;
	float inflow;
	float outflow;
	int   cores;
};

int  soil_open(struct soil *);
void soil_sample(struct soil *, float elapsed);

#endif
