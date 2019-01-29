#pragma once
#ifndef __STRATEGY__
#define __STRATEGY__

#include <vector>

#include "migration/page_table.hpp"
#include "migration/migration_cell.hpp"

class strategy {
	// Core functions to select the page/thread to migrate and where
public:
	// One process strategies
	std::vector<migration_cell_t> get_pages_to_migrate (page_table_t & page_t);
	std::vector<migration_cell_t> get_threads_to_migrate (page_table_t & page_t);

	// Global strategies
	std::vector<migration_cell_t> get_pages_to_migrate (std::map<pid_t, page_table_t> & page_ts);
	std::vector<migration_cell_t> get_threads_to_migrate (std::map<pid_t, page_table_t> & page_ts);
};

#endif
