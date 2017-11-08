#pragma once

#include "strategy.h"

typedef struct first_touch : strategy {
	// Only valid for pages
	vector<migration_cell_t> get_pages_to_migrate(page_table_t *page_t);
} first_touch_t;

