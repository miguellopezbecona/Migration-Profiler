#pragma once

#include "strategy.h"

typedef struct random : strategy {
	// The following should not be written because it should inherit it from strategy
	vector<migration_cell_t> get_pages_to_migrate(page_table_t *page_t);
	vector<migration_cell_t> get_threads_to_migrate(page_table_t *page_t);
} random_t;

