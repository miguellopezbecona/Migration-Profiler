#pragma once

#include <vector>
using namespace std;

#include "../migration/page_table.h"
#include "../migration/migration_cell.h"

typedef struct strategy {
	// Core functions to select the page/thread to migrate and where

	// One process strategies
	vector<migration_cell_t> get_pages_to_migrate(page_table_t *page_t);
	vector<migration_cell_t> get_threads_to_migrate(page_table_t *page_t);

	// Global strategies
	vector<migration_cell_t> get_pages_to_migrate(map<pid_t, page_table_t> *page_ts);
	vector<migration_cell_t> get_threads_to_migrate(map<pid_t, page_table_t> *page_ts);
} strategy_t;
