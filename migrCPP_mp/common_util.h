#ifndef __COMMON_UTIL_H__
#define __COMMON_UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>

typedef struct my_pebs_sample{
	uint64_t iip;
	int pid;
	int tid;
	uint64_t time;
	uint64_t sample_addr;
//	uint64_t id;
	uint32_t cpu;
//	uint64_t period;
	uint64_t weight;
	uint64_t time_enabled;
	uint64_t time_running;
	uint64_t nr;
	uint64_t *values;
//	uint64_t values[4];
	uint64_t dsrc;
	//char type='u'; //used to say what kind of sample it is, may be 'u', unknown

	//For use with no output
	void print(FILE *fp);
	void print_for_3DyRM(FILE *fp);
} my_pebs_sample_t;

// Defined in my_profiler.c
int get_time_value();
int time_go_up();
int time_go_down();

#endif
