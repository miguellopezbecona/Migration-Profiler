#pragma once
#ifndef __FIRST_TOUCH_STRATEGY__
#define __FIRST_TOUCH_STRATEGY__

#include "strategy.hpp"

class first_touch_t : public strategy {
public:
	// Only valid for pages

	// Migrates pages if they were accessed by a thread residing in a different memory node
	std::vector<migration_cell_t> get_pages_to_migrate (const page_table_t & page_t) const {
		std::vector<migration_cell_t> ret;

		// Loops over all the pages
		for (const auto & t_it : page_t.page_node_map) {
			const auto & pd = t_it.second;

			// Gets memory node of last CPU access
			const auto cpu_node = system_struct_t::get_cpu_memory_node(pd.last_cpu_access);

			// Compares to page location and adds to migration list if different
			const auto page_node = pd.current_node;
			if (cpu_node != page_node) {
				const auto addr = t_it.first;
				ret.push_back(migration_cell_t(addr, cpu_node, page_t.pid, false));
			}
		}

		return ret;
	}
};

#endif
