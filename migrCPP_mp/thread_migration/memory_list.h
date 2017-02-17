#ifndef __MEMORY_LIST_H__
#define __MEMORY_LIST_H__

#include <stdio.h>
#include <inttypes.h>

#include <vector> // std::vector
using namespace std;

typedef struct memory_data_cell{
	uint32_t cpu;
	int pid;
	int tid;
	uint64_t addr;
	uint64_t latency;
	uint64_t dsrc;
	uint64_t time;

	memory_data_cell(){}
	memory_data_cell(uint32_t cpu, int pid, int tid, uint64_t addr, uint64_t latency, uint64_t dsrc, uint64_t time);
	void print();
	bool is_cache_miss();
} memory_data_cell_t;

typedef struct memory_data_list{
	vector<memory_data_cell_t> list;

	void add_cell(uint32_t cpu, int pid, int tid, uint64_t addr, uint64_t latency, uint64_t dsrc, uint64_t time);
	void clear();
	void print();
} memory_data_list_t;

#endif
