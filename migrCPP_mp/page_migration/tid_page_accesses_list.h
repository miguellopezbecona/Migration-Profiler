#ifndef __TID_PAGE_ACCESSES_LIST_H__
#define __TID_PAGE_ACCESSES_LIST_H__

#include <stdio.h>
#include <stdlib.h>

#include <vector>
using namespace std;

typedef struct tid_page_accesses_cell{
	pid_t tid;
	int cpu; //might not be needed later, when we mix both migrations
	float mean_latency;
	float mean_latency_reset;
	unsigned accesses;
	unsigned accesses_reset;

	tid_page_accesses_cell(){}
	tid_page_accesses_cell(pid_t tid, float latency, int cpu);
	int update(float latency, int cpu);
	int reset();
	void print();
} tid_page_accesses_cell_t;

typedef struct tid_page_accesses_list{
	vector<tid_page_accesses_cell_t> list;

	int add_cell(pid_t tid, float latency, int cpu);
	tid_page_accesses_cell_t* get_cell(pid_t tid);
	int reset();
	void clear();
	void print();
} tid_page_accesses_list_t;

#endif
