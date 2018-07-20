#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

using namespace std;
#include <vector>

#ifdef JUST_PROFILE_ENERGY
#include "rapl/energy_data.h"
#endif

// For omitting usually useless columns while printing: IIP, TIME_E, TIME_R, MEM_OPS, DSRC
#define SIMPL_PRINT

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

	#ifdef JUST_PROFILE_ENERGY
	double* energies;
	void add_energy_data();
	#endif

	#ifdef JUST_PROFILE
	static int max_nr;
	static vector<char*> inst_subevent_names;

	static void add_subevent_name(char* name);
	#endif

	bool is_mem_sample() const;
	void print(FILE *fp) const;
	static void print_header(FILE *fp);
} my_pebs_sample_t;

