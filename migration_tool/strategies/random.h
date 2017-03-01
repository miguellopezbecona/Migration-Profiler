#pragma once

#include "strategy.h"

typedef struct random : strategy {
	// The following should not be written because it should inherit it from strategy
	migration_cell_t get_page_to_migrate(page_table_t *page_t);
	migration_cell_t get_thread_to_migrate(page_table_t *page_t);
} random_t;

