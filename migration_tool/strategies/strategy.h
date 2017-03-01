#pragma once

#include "../migration/migration_cell.h"

typedef struct strategy {
	// Cores function to select the page/thread to migrate and where
	migration_cell_t get_page_to_migrate(page_table_t *page_t);
	migration_cell_t get_thread_to_migrate(page_table_t *page_t);
} strategy_t;
