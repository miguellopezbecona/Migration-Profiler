#pragma once

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>

typedef struct my_pebs_sample{
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

	//For use with no output
	void print(FILE *fp);
	void print_for_3DyRM(FILE *fp);
} my_pebs_sample_t;

// Defined in my_profiler.c
int get_time_value();
int time_go_up();
int time_go_down();

