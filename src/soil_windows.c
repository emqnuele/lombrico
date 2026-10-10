#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>

#include "soil.h"

static uint64_t busy_was, total_was;
static uint64_t in_was, out_was;
static bool primed;

static uint64_t
ticks_of(FILETIME t)
{
	return ((uint64_t)t.dwHighDateTime << 32) | t.dwLowDateTime;
}

static void
ticks(uint64_t *busy, uint64_t *total)
{
	FILETIME idle, kernel, user;

	if (!GetSystemTimes(&idle, &kernel, &user)) {
		return;
	}
	*total = ticks_of(kernel) + ticks_of(user);
	*busy = *total - ticks_of(idle);
}

static float
memory_used(void)
{
	MEMORYSTATUSEX status = { .dwLength = sizeof status };

	if (!GlobalMemoryStatusEx(&status) || status.ullTotalPhys == 0) {
		return 0;
	}
	return 1.0f - (float)status.ullAvailPhys / (float)status.ullTotalPhys;
}

static void
traffic(uint64_t *in, uint64_t *out)
{
	MIB_IF_TABLE2 *table;
	const MIB_IF_ROW2 *row;
	ULONG i;

	if (GetIfTable2(&table) != NO_ERROR) {
		return;
	}

	*in = *out = 0;
	for (i = 0; i < table->NumEntries; i++) {
		row = &table->Table[i];
		if (row->Type == IF_TYPE_SOFTWARE_LOOPBACK ||
		    row->InterfaceAndOperStatusFlags.FilterInterface ||
		    !row->InterfaceAndOperStatusFlags.HardwareInterface) {
			continue;
		}
		*in += row->InOctets;
		*out += row->OutOctets;
	}
	FreeMibTable(table);
}

static float
rate(uint64_t now, uint64_t was, float elapsed)
{
	return now > was ? (float)(now - was) / elapsed : 0;
}

int
soil_open(struct soil *s)
{
	SYSTEM_INFO info;

	memset(s, 0, sizeof *s);
	GetSystemInfo(&info);
	s->cores = (int)info.dwNumberOfProcessors;

	s->load = -1;
	soil_sample(s, 0);

	return 0;
}

void
soil_sample(struct soil *s, float elapsed)
{
	uint64_t busy = busy_was, total = total_was, in = in_was, out = out_was;

	ticks(&busy, &total);
	traffic(&in, &out);

	if (primed && elapsed > 0) {
		s->cpu = total > total_was ?
		    (float)(busy - busy_was) / (float)(total - total_was) : s->cpu;
		s->inflow = rate(in, in_was, elapsed);
		s->outflow = rate(out, out_was, elapsed);
	}
	s->memory = memory_used();

	busy_was = busy;
	total_was = total;
	in_was = in;
	out_was = out;
	primed = true;
}
