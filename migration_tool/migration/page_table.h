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

typedef struct table_cell {
	vector<int> latencies;
	unsigned cache_misses;

	table_cell(){}
	table_cell(int latency, bool is_cache_miss);
	void print();
	void update(int latency, bool is_cache_miss);
} table_cell_t;

// For threads and memory pages
typedef struct perf_data {
	unsigned char current_place; // Mem node for pages or core for TIDs
	unsigned short int num_uniq_accesses;
	unsigned short int num_acs_thres; // Always equal or lower than num_uniq_accesses
	int min_latency;
	int median_latency;
	int max_latency;

	perf_data(){}
	void print() const;
} perf_data_t;

typedef map<long int, table_cell_t> column; // Each column of the table, defined with typedef for readibility

typedef struct page_table {
	vector<column> table; // Matrix/table where rows are threads (uses index from tid_index) and columns are indexed by page address
	set<long int> uniq_addrs; // All different addresses used in this table, useful for heatmap printing
	map<int, short> tid_index; // Translates TID to row index

	map<long int, perf_data_t> page_node_map; // Maps page address to memory node and other data
	map<int, perf_data_t> tid_node_map; // Maps TID to memory node and other data

	pid_t pid;

	// No constructor because SYS_NUM_OF_CORES is not defined at constructor call time
	void init(pid_t p){
		pid = p;
		table.resize(SYS_NUM_OF_CORES);
	}
	int add_cell(long int page_addr, int current_node, pid_t tid, int latency, int cpu, int cpu_node, bool is_cache_miss);
	bool contains_addr(long int page_addr, int cpu);
	table_cell_t* get_cell(long int page_addr, int cpu);
	vector<int> get_latencies_from_cell(long int page_addr, int cpu);
	int reset_column(long int page_addr, int current_node);
	void remove_inactive_tids();
	void clear();
	void print();

	void calculate_performance_page(int threshold);
	void calculate_performance_tid(int threshold);
	void calculate_performance(int threshold); // Intended to unify the two above
	void print_performance();

	// More info on the definitions in .c file
	void print_heatmaps(FILE **fps, int num_fps);
	void print_alt_graph(FILE *fp);
	void print_table1();
	void print_table2();
} page_table_t;

