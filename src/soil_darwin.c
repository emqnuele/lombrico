#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mach/mach.h>
#include <net/if.h>
#include <net/route.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include "soil.h"

static host_t host;
static vm_size_t page;
static uint64_t memory_total;

static uint64_t busy_was, total_was;
static uint64_t in_was, out_was;
static bool primed;

static char *routes;
static size_t routes_cap;

static void
ticks(uint64_t *busy, uint64_t *total)
{
	host_cpu_load_info_data_t load;
	mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

	if (host_statistics(host, HOST_CPU_LOAD_INFO, (host_info_t)&load,
	    &count) != KERN_SUCCESS) {
		*busy = busy_was;
		*total = total_was;
		return;
	}

	*busy = (uint64_t)load.cpu_ticks[CPU_STATE_USER] +
	    load.cpu_ticks[CPU_STATE_SYSTEM] +
	    load.cpu_ticks[CPU_STATE_NICE];
	*total = *busy + load.cpu_ticks[CPU_STATE_IDLE];
}

static float
memory_used(void)
{
	vm_statistics64_data_t vm;
	mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
	uint64_t pages;

	if (memory_total == 0 || host_statistics64(host, HOST_VM_INFO64,
	    (host_info64_t)&vm, &count) != KERN_SUCCESS) {
		return 0;
	}

	pages = (uint64_t)vm.internal_page_count - vm.purgeable_count +
	    vm.wire_count + vm.compressor_page_count;

	return (float)((double)(pages * page) / (double)memory_total);
}

static void
traffic(uint64_t *in, uint64_t *out)
{
	int mib[] = { CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST2, 0 };
	const struct if_msghdr2 *ifm;
	size_t len;
	char *next, *grown;

	*in = in_was;
	*out = out_was;

	if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0) {
		return;
	}
	if (len > routes_cap) {
		grown = realloc(routes, len);
		if (grown == NULL) {
			return;
		}
		routes = grown;
		routes_cap = len;
	}
	if (sysctl(mib, 6, routes, &len, NULL, 0) < 0) {
		return;
	}

	*in = *out = 0;
	for (next = routes; next < routes + len; next += ifm->ifm_msglen) {
		ifm = (const struct if_msghdr2 *)(void *)next;
		if (ifm->ifm_msglen == 0) {
			break;
		}
		if (ifm->ifm_type != RTM_IFINFO2 || (ifm->ifm_flags & IFF_LOOPBACK)) {
			continue;
		}
		*in += ifm->ifm_data.ifi_ibytes;
		*out += ifm->ifm_data.ifi_obytes;
	}
}

static float
rate(uint64_t now, uint64_t was, float elapsed)
{
	return now > was ? (float)(now - was) / elapsed : 0;
}

int
soil_open(struct soil *s)
{
	int cores = 1;
	size_t len;

	memset(s, 0, sizeof *s);
	host = mach_host_self();

	if (host_page_size(host, &page) != KERN_SUCCESS) {
		return -1;
	}
	len = sizeof memory_total;
	if (sysctlbyname("hw.memsize", &memory_total, &len, NULL, 0) < 0) {
		return -1;
	}
	len = sizeof cores;
	sysctlbyname("hw.logicalcpu", &cores, &len, NULL, 0);
	s->cores = cores;

	soil_sample(s, 0);

	return 0;
}

void
soil_sample(struct soil *s, float elapsed)
{
	uint64_t busy, total, in, out;
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
