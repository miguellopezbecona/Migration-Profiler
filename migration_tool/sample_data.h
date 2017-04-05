#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

typedef struct my_pebs_sample {
	uint64_t iip;
	int pid;
	int tid;
	uint64_t time;
	uint64_t sample_addr;
	uint32_t cpu;
	uint64_t weight;
	uint64_t time_enabled;
	uint64_t time_running;
	uint64_t nr;
	uint64_t *values;
	uint64_t dsrc;

	bool is_mem_sample() const;
	void print(FILE *fp) const;
	void print_for_3DyRM(FILE *fp) const;
	static void print_header(FILE *fp);
} my_pebs_sample_t;

