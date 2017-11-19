#pragma once

#include "system_struct.h"

// Might be used in the future to ponderate performance calculation. Not used right now
const int DYRM_ALPHA = 1;
const int DYRM_BETA = 1;
const int DYRM_GAMMA = 1;

const int PERFORMANCE_INVALID_VALUE = -1;
const int DEFAULT_LATENCY_FOR_CONSISTENCY = 1000; // This might be used when we do not have a measured latency

// Base perf data for threads and memory pages. Now is mainly used for storing page locations. It may be more relevant in the future
typedef struct base_perf_data {
	unsigned short num_uniq_accesses;
	unsigned short num_acs_thres; // Always equal or lower than num_uniq_accesses
	unsigned short min_latency;
	unsigned short median_latency;
	unsigned short max_latency;

	base_perf_data(){
		num_acs_thres = 0;
	}

	void print() const;
} base_perf_data_t;

typedef struct pg_perf_data : base_perf_data_t {
	char current_node;
	short last_cpu_access;

	vector<unsigned short> acs_per_node; // Number of accesses from threads per memory node

	pg_perf_data(){
		num_acs_thres = 0;

		vector<unsigned short> v(system_struct_t::NUM_OF_MEMORIES, 0);
		acs_per_node = v;
	}

	void print() const;
} pg_perf_data_t;

typedef struct th_perf_data : base_perf_data_t {
} th_perf_data_t;

// Based on Óscar's work in this PhD. This would be associated to each thread from the PID associated to the table
typedef struct rm3d_data {
	const int CACHE_LINE_SIZE = 64;

	bool active; // A TID is considered active if we received samples from it in the current iteration

	// Sum of inst sample fields, per core
	vector<long int> insts;
	vector<long int> reqs;
	vector<long int> times;

	vector<double> v_perfs; // 3DyRM performance per memory node
	unsigned char index_last_node_calc; // Used to get last_3DyRM_perf from Óscar's code

	rm3d_data(){
		insts.resize(system_struct_t::NUM_OF_CPUS);
		reqs.resize(system_struct_t::NUM_OF_CPUS);
		times.resize(system_struct_t::NUM_OF_CPUS);
		v_perfs.resize(system_struct_t::NUM_OF_MEMORIES);

		reset();
	}

	void add_data(int cpu, long int insts, long int reqs, long int time);
	void calc_perf(double mean_latency);
	void reset();

	void print() const;
} rm3d_data_t;

