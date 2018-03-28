#pragma once

#include "strategy.h"

typedef struct energy_str : strategy {
	vector<migration_cell_t> get_migrations(map<pid_t, page_table_t> *page_ts);

	vector<migration_cell_t> get_threads_to_migrate(map<pid_t, page_table_t> *page_ts);
	vector<migration_cell_t> get_pages_to_migrate(map<pid_t, page_table_t> *page_ts);
} energy_str_t;

