#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "soil.h"

static uint64_t busy_was, total_was;
static uint64_t in_was, out_was;
static bool primed;

static bool
ticks(uint64_t *busy, uint64_t *total)
{
	unsigned long long user = 0, nice = 0, system = 0, idle = 0;
	unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0;
	FILE *stat = fopen("/proc/stat", "r");
	int found;

	if (stat == NULL) {
		return false;
	}
	found = fscanf(stat, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
	    &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
	fclose(stat);

	if (found < 4) {
		return false;
	}
	*busy = user + nice + system + irq + softirq + steal;
	*total = *busy + idle + iowait;
	return true;
}

static float
memory_used(void)
{
	unsigned long long total = 0, available = 0, value;
	FILE *info = fopen("/proc/meminfo", "r");
	char line[128];

	if (info == NULL) {
		return 0;
	}
	while (fgets(line, sizeof line, info) != NULL) {
		if (sscanf(line, "MemTotal: %llu kB", &value) == 1) {
			total = value;
		} else if (sscanf(line, "MemAvailable: %llu kB", &value) == 1) {
			available = value;
		}
	}
	fclose(info);

	return total > 0 ? 1.0f - (float)available / (float)total : 0;
}

static bool
traffic(uint64_t *in, uint64_t *out)
{
	unsigned long long received, sent;
	FILE *dev = fopen("/proc/net/dev", "r");
	char line[512], name[64];

	if (dev == NULL) {
		return false;
	}

	*in = *out = 0;
	while (fgets(line, sizeof line, dev) != NULL) {
		if (sscanf(line, " %63[^:]: %llu %*u %*u %*u %*u %*u %*u %*u %llu",
		    name, &received, &sent) != 3 || strcmp(name, "lo") == 0) {
			continue;
		}
		*in += received;
		*out += sent;
	}
	fclose(dev);
	return true;
}

static float
rate(uint64_t now, uint64_t was, float elapsed)
{
	return now > was ? (float)(now - was) / elapsed : 0;
}

int
soil_open(struct soil *s)
{
	long cores = sysconf(_SC_NPROCESSORS_ONLN);

	memset(s, 0, sizeof *s);
	s->cores = cores > 0 ? (int)cores : 1;
	soil_sample(s, 0);

	return 0;
}

void
soil_sample(struct soil *s, float elapsed)
{
	uint64_t busy = busy_was, total = total_was, in = in_was, out = out_was;
	double load;

	ticks(&busy, &total);
	traffic(&in, &out);

	if (primed && elapsed > 0) {
		s->cpu = total > total_was ?
		    (float)(busy - busy_was) / (float)(total - total_was) : s->cpu;
		s->inflow = rate(in, in_was, elapsed);
		s->outflow = rate(out, out_was, elapsed);
	}

	s->memory = memory_used();
	if (getloadavg(&load, 1) == 1) {
		s->load = (float)load;
	}

	busy_was = busy;
	total_was = total;
	in_was = in;
	out_was = out;
	primed = true;
}
