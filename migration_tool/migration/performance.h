#pragma once

#include "system_struct.h"

// Might be used in the future to ponderate performance calculation. Not used right now
const int DYRM_ALPHA = 1;
const int DYRM_BETA = 1;
const int DYRM_GAMMA = 1;

const int PERFORMANCE_INVALID_VALUE = -1;

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

// Based on Óscar's work in his PhD. This would be associated to each thread from the PID associated to the table
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

/*** Experimental structures ***/

// The content of each cell of these new tables
typedef struct perf_cell {
	// Right now, we just have into account the number of acceses and the mean latency
	unsigned int num_acs;
	double mean_lat;

	perf_cell();
	perf_cell(double lat);
	void update_mlat(double lat);

	bool is_filled() const;
	void print() const;
} perf_cell_t;

// Experimental table to store historical performance for each thread/memory page in each CPU/node
typedef vector<perf_cell_t> perf_col; // Readibility
typedef struct perf_table {
	static double alfa; // For future aging technique

	unsigned short coln; // NUM_OF_MEMORIES or NUM_OF_CPUS
	map<long int, perf_col> table; // Dimensions: TID/page addr and CPU/node

	perf_table();
	perf_table(unsigned short n);
	~perf_table();
	bool has_row(long int key);
	void remove_row(long int key);
	void add_data(long int key, int col_num, double lat);
	void print() const;

	// Some future functions could be coded such as: how many CPUs have data for a given TID?, which CPU has the most/least accesses?, etc.

} perf_table_t;
extern perf_table_t tid_cpu_table;
