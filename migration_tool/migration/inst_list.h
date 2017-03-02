#pragma once

#include <stdio.h> // printf
#include <string.h> // memset
#include <inttypes.h>

#include <vector> // std::vector
using namespace std;

#include "system_struct.h" // SYS_NUM_OF_CORES

typedef struct inst_data_cell{
	uint32_t cpu;
	int pid;
	int tid;
	uint64_t inst;
	uint64_t req_dr;
	uint64_t time;

	inst_data_cell(){}
	inst_data_cell(uint32_t cpu, int pid, int tid, uint64_t inst, uint64_t req_dr, uint64_t time);
	void print();
} inst_data_cell_t;

typedef struct inst_data_list{
	vector<inst_data_cell_t> list;

	inst_data_list(){
		list.reserve(100); // Speeds up doing preallocation
	}
	void add_cell(uint32_t cpu, int pid, int tid, uint64_t inst, uint64_t req_dr, uint64_t time);
	void clear();
	void print();
	int create_increments();
} inst_data_list_t;

