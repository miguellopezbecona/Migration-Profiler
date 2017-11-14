#pragma once

#include "strategy.h"

#define TICKETS_MEM_CELL_NO_VALID -1
#define TICKETS_MEM_CELL_WORSE 1
#define TICKETS_MEM_CELL_NO_DATA 2
#define TICKETS_MEM_CELL_BETTER 4
#define TICKETS_MEM_CELL_WORSE_2 1
#define TICKETS_MEM_CELL_NO_DATA_2 2
#define TICKETS_MEM_CELL_BETTER_2 4
#define TICKETS_FREE_CORE 3

#define DYRM_ALPHA 1
#define DYRM_BETA 1
#define DYRM_GAMMA 1

#define PERFORMANCE_INVALID_VALUE -1
#define DEFAULT_LATENCY_FOR_CONSISTENCY 1000 // This might be used when we do not have a measured latency

typedef struct annealing : strategy {
	// Only valid for threads
	vector<migration_cell_t> get_threads_to_migrate(page_table_t *page_t);
} annealing_t;

