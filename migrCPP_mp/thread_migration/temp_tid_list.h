#ifndef __TEMP_TID_LIST_H__
#define __TEMP_TID_LIST_H__

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "system_struct.h" // SYS_NUM_OF_CORES

#include <vector>
using namespace std;

typedef struct temp_tid_cell{
	pid_t pid;
	pid_t tid;
	uint32_t last_core;
	uint64_t inst[SYS_NUM_OF_CORES];
	uint64_t req_dr[SYS_NUM_OF_CORES];
	uint64_t elapsed[SYS_NUM_OF_CORES];
	unsigned int n_o_inst_samples[SYS_NUM_OF_CORES];
	uint64_t latency[SYS_NUM_OF_CORES];
	unsigned int latency_samples[SYS_NUM_OF_CORES];

	temp_tid_cell(){}
	temp_tid_cell(uint32_t cpu, pid_t pid, pid_t tid, uint64_t inst, uint64_t req_dr, uint64_t elapsed);
	int update_cell(uint32_t cpu, uint64_t inst, uint64_t req_dr, uint64_t elapsed);
	int update_latency(uint32_t cpu, uint64_t latency);
	void print();
} temp_tid_cell_t;

typedef struct temp_tid_list{
	vector<temp_tid_cell_t> list;

	int add_cell(uint32_t cpu, pid_t pid, pid_t tid, uint64_t inst, uint64_t req_dr, uint64_t elapsed);
	temp_tid_cell_t* get_cell(pid_t tid);
	void clear();
	void print();
} temp_tid_list_t;

#endif
