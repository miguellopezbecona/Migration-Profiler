#pragma once

#include "strategy.h"

const int TICKETS_MEM_CELL_WORSE[] = {1, 1};
const int TICKETS_MEM_CELL_NO_DATA[] = {2, 2};
const int TICKETS_MEM_CELL_BETTER[] = {4, 4};
const int TICKETS_FREE_CORE = 3;

// Might be used to ponderate performance calculation. Not used right now
const int DYRM_ALPHA = 1;
const int DYRM_BETA = 1;
const int DYRM_GAMMA = 1;

const int PERFORMANCE_INVALID_VALUE = -1;
const int DEFAULT_LATENCY_FOR_CONSISTENCY = 1000; // This might be used when we do not have a measured latency

#define ANNEALING_PRINT

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
	vector<migration_cell_t> get_threads_to_migrate(page_table_t *page_t);
} annealing_t;

