#pragma once

#include "strategy.h"

const int TICKETS_MEM_CELL_WORSE[] = {1, 1};
const int TICKETS_MEM_CELL_NO_DATA[] = {2, 2};
const int TICKETS_MEM_CELL_BETTER[] = {4, 4};
const int TICKETS_FREE_CORE = 3;

#define ANNEALING_PRINT
//#define ANNEALING_PRINT_MORE_DETAILS

// Special structure which takes into account that each possible choice is a simple migration or an interchange, and has tickets associated to it
typedef struct labeled_migr {
	vector<migration_cell_t> potential_migr; // 1 element for simple migration, 2 for interchange
	int tickets;

	labeled_migr(){}
	labeled_migr(migration_cell_t mc, int t);
	labeled_migr(migration_cell_t mc1, migration_cell_t mc2, int t);
	bool is_interchange() const;
	void prepare_for_undo();
	void print() const;
} labeled_migr_t;

typedef struct annealing : strategy {
	// Only valid for threads
	vector<migration_cell_t> get_threads_to_migrate(map<pid_t, page_table_t> *page_ts);
} annealing_t;

