#ifndef __TID_LIST_H__
#define __TID_LIST_H__

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sched.h>

//for pow()
#include <math.h>

#include "tid_performance.h"

#include <tr1/unordered_map>
using namespace std;

typedef struct tid_cell{
	pid_t tid;
	bool pinned;
	uint32_t last_core;
	int assigned_core;
	cpu_set_t affinity;
	unsigned int step_last_migrated;
	bool active;
	tid_performance_t performance;

	tid_cell(){}
	tid_cell(pid_t tid, uint32_t core);
	void print();
	double get_last_performance();
	int update_performance(int core, unsigned int step, unsigned int instructions, float flops, float flopb, float latency);
} tid_cell_t;

typedef struct tid_list{
	tr1::unordered_map<pid_t, tid_cell_t> map;

	void add_cell(pid_t tid, uint32_t core);
	tid_cell_t* get_cell(pid_t tid);
	int reset_performance();
	int set_inactive();
	void clear();
	void print();

	tid_cell_t* get_cell_assigned_in_core(int core);
} tid_list_t;

#endif
