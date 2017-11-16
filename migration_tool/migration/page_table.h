#pragma once

#include <stdio.h>

// Data structures
#include <vector>
#include <map>
#include <set>

#include <numeric> // accumulate
#include <algorithm> // min_element and max_element
using namespace std;

#include "system_struct.h" // Needs to know the system
#include "../utils.h"

// Might be used in the future to ponderate performance calculation. Not used right now
const int DYRM_ALPHA = 1;
const int DYRM_BETA = 1;
const int DYRM_GAMMA = 1;

const int PERFORMANCE_INVALID_VALUE = -1;
const int DEFAULT_LATENCY_FOR_CONSISTENCY = 1000; // This might be used when we do not have a measured latency


typedef struct table_cell {
	vector<int> latencies;
	unsigned cache_misses;
	unsigned last_cpu_access;

	table_cell(){}
	table_cell(int latency, bool is_cache_miss, int cpu);
	void print();
	void update(int latency, bool is_cache_miss, int cpu);
} table_cell_t;

// For threads and memory pages. [Not actually used (except for page locations). Coded for a specific test. It may be discarded]
typedef struct perf_data {
	// Reuse migration_cell??
	unsigned char current_place; // Mem node for pages or core for TIDs

	vector<unsigned short> acs_per_node; // Number of accesses from threads per memory node

	unsigned short num_uniq_accesses;
	unsigned short num_acs_thres; // Always equal or lower than num_uniq_accesses
	int min_latency;
	int median_latency;
	int max_latency;

	perf_data(){
		num_acs_thres = 0;

		vector<unsigned short> v(system_struct_t::NUM_OF_MEMORIES, 0);
		acs_per_node = v;
	}

	void print() const;
} perf_data_t;

// Based on Óscar's work in this PhD. This would be associated to each thread for the PID
typedef struct perform_data {
	const int CACHE_LINE_SIZE = 64;

	bool active; // A TID is considered active if we received samples from it in the current iteration

	// Sum of fields per core
	vector<long int> insts;
	vector<long int> reqs;
	vector<long int> times;

	vector<double> v_perfs; // 3DyRM performance per memory node
	int index_last_node_calc; // Used to get last_3DyRM_perf from Óscar's code

	perform_data(){
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
} perform_data_t;

typedef map<long int, table_cell_t> column; // Each column of the table, defined with typedef for readibility

typedef struct page_table {
	vector<column> table; // Matrix/table where rows are threads (uses index from tid_index) and columns are indexed by page address
	set<long int> uniq_addrs; // All different addresses used in this table, useful for heatmap printing
	map<int, short> tid_index; // Translates TID to row index

	map<long int, perf_data_t> page_node_map; // Maps page address to memory node and other data

	// Using long int as key to use along the previous one as function parameter
	map<long int, perf_data_t> tid_node_map; // Maps TID to memory node and other data

	map<pid_t, perform_data_t> perf_per_tid; // Maps TID to Óscar's performance data

	pid_t pid;

	page_table(){}
	page_table(pid_t p);
	~page_table();

	int add_cell(long int page_addr, int current_node, pid_t tid, int latency, int cpu, int cpu_node, bool is_cache_miss);
	bool contains_addr(long int page_addr, int cpu);
	table_cell_t* get_cell(long int page_addr, int cpu);
	vector<int> get_latencies_from_cell(long int page_addr, int cpu);
	void remove_tid(pid_t tid);
	void remove_inactive_tids();
	void print();

	void calculate_performance_page(int threshold);
	void calculate_performance_tid(int threshold);
	void calculate_performance(int threshold); // Intended to unify the two above
	void print_performance() const;

	// Code made for replicate Óscar's work
	void add_inst_data_for_tid(pid_t tid, int core, long int insts, long int req_dr, long int time);
	void calc_perf(); // Óscar's definition of performance
	pid_t normalize_perf_and_get_worst_thread();
	void reset_performance();
	double get_total_performance();
	vector<double> get_perf_data(pid_t tid);

	double get_mean_acs_to_pages();
	double get_mean_lat_to_pages();

	// More info about the definitions in .c file
	void print_heatmaps(FILE **fps, int num_fps);
	void print_alt_graph(FILE *fp);
	void print_table1();
	void print_table2();
} page_table_t;

