#pragma once

#include <stdio.h>

// Data structures
#include <vector>
#include <map>
#include <set>

#include <numeric> // accumulate
#include <algorithm> // min_element and max_element
using namespace std;

/* USES tid_page_accesses_list.h */
#include "tid_page_accesses_list.h"

// Needs to know the system
#include "../thread_migration/system_struct.h"

typedef struct table_cell {
	vector<float> latencies;

	//tid_page_accesses_list_t accesses_l;
	unsigned cache_misses;

	table_cell(){}
	table_cell(pid_t tid, float latency, int cpu, bool is_cache_miss);
	void print();
	int update(pid_t tid, float latency, int cpu, bool is_cache_miss);
} table_cell_t;

// For threads and memory pages
typedef struct perf_data {
	unsigned char current_mem_node;
	unsigned short int num_uniq_accesses;
	unsigned short int num_acs_thres; // Always equal or lower than num_uniq_accesses
	float avg_latency;

	perf_data(){}
	void print();
} perf_data_t;

typedef struct page_table{
	map<long int, table_cell_t> table[SYS_NUM_OF_CORES]; // Matrix/table where rows are cores (static) and columns are indexed by page address
	set<long int> uniq_addrs; // All different addresses, useful for heatmap printing

	map<long int, perf_data_t> page_node_map; // Maps page address to memory node and other data
	map<int, perf_data_t> tid_node_map; // Maps TID to memory node and other data

	pid_t pid;

	int add_cell(long int page_addr, int current_node, pid_t tid, float latency, int cpu, int cpu_node, bool is_cache_miss);
	bool contains_addr(long int page_addr, int cpu);
	table_cell_t* get_cell(long int page_addr, int cpu);
	vector<float> get_latencies_from_cell(long int page_addr, int cpu);
	int reset_cell(long int page_addr, int current_node);
	void clear();
	void print();

	void calculate_performance_page(double threshold);
	void calculate_performance_tid(double threshold);
	void calculate_performance(double threshold); // Intended to unify the two above
	void print_performance();

	// More info on the definitions in .c file
	void print_heatmaps(FILE **fps, int num_fps);
	void print_alt_graph(FILE *fp);
	void print_table1();
	void print_table2();
} page_table_t;

