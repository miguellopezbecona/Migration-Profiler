#pragma once

#include <vector>
using namespace std;

#include "../migration/migration_cell.h"

typedef struct strategy {
	// Cores function to select the page/thread to migrate and where
	vector<migration_cell_t> get_pages_to_migrate(page_table_t *page_t);
	vector<migration_cell_t> get_threads_to_migrate(page_table_t *page_t);
} strategy_t;
