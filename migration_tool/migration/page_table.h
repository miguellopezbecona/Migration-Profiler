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
#include "migration_cell.h"
#include "performance.h"
#include "../utils.h"

typedef struct table_cell {
	vector<int> latencies;
	unsigned cache_misses;

	table_cell(){}
	table_cell(int latency, bool is_cache_miss);
	void print();
	void update(int latency, bool is_cache_miss);
} table_cell_t;

typedef map<long int, table_cell_t> column; // Each column of the table, defined with typedef for readibility

typedef struct page_table {
	vector<column> table; // Matrix/table where rows are threads (uses index from tid_index) and columns are indexed by page address
	set<long int> uniq_addrs; // All different addresses used in this table, useful for heatmap printing
	map<int, short> tid_index; // Translates TID to row index

	map<long int, pg_perf_data_t> page_node_map; // Maps page address to memory node and other data (may be redundant with page_node_table)

	map<pid_t, rm3d_data_t> perf_per_tid; // Maps TID to Óscar's performance data

	perf_table_t page_node_table; // Experimental table for memory pages

	pid_t pid;

	page_table(){}
	page_table(pid_t p);
	~page_table();

	int add_cell(long int page_addr, int current_node, pid_t tid, int latency, int cpu, int cpu_node, bool is_cache_miss);
	bool contains_addr(long int page_addr, int cpu);
	table_cell_t* get_cell(long int page_addr, int cpu);
	vector<int> get_latencies_from_cell(long int page_addr, int cpu);
	void remove_tid(pid_t tid);
	void remove_finished_tids(bool unpin_3drminactive_tids);
	vector<pid_t> get_tids() const;
	void print();

	void calculate_performance_page(int threshold);
	void print_performance() const;
	void update_page_locations(vector<migration_cell_t> pg_migr);

	// Code made for replicating Óscar's work
	void add_inst_data_for_tid(pid_t tid, int core, long int insts, long int req_dr, long int time);
	void calc_perf(); // Óscar's definition of performance
	pid_t normalize_perf_and_get_worst_thread();
	void reset_performance();
	double get_total_performance();
	vector<double> get_perf_data(pid_t tid);

	vector<int> get_lats_for_tid(pid_t tid);
	double get_mean_acs_to_pages();
	double get_mean_lat_to_pages();

	// More info about the definitions in source file
	void print_heatmaps(FILE **fps, int num_fps);
	void print_alt_graph(FILE *fp);
} page_table_t;

