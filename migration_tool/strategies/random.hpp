#pragma once
#ifndef __RANDOM_STRATEGY__
#define __RANDOM_STRATEGY__

#include <iostream>

#include "strategy.hpp"

class random_t : public strategy {
public:
	// The following should not be written because it should inherit it from strategy
	// Picks random memory page and memory node destination
	std::vector<migration_cell_t> get_pages_to_migrate (const page_table_t & page_t) const {
		std::vector<migration_cell_t> ret(1);

		// Searches random page address in set
		const std::set<long int> & s = page_t.uniq_addrs;
		const int pos = rand() % s.size();
		const auto page_addr = *(std::next(s.begin(), pos));

		// Creates migration cell with data
		const unsigned char mem_node = rand() % system_struct_t::NUM_OF_MEMORIES;
		const migration_cell_t mc(page_addr, mem_node, page_t.pid, false);

		ret.push_back(mc);
		return ret;
	}

	// Picks random thread and core destination
	std::vector<migration_cell_t> get_threads_to_migrate (const page_table_t & page_t) const {
		std::vector<migration_cell_t> ret(1);

		// Searches random TID in map
		const std::map<int, short> & m = page_t.tid_index;
		int pos = rand() % m.size();
		const auto & page_addr = *(std::next(m.begin(), pos));

		// Creates migration cell with data
		const long int tid = page_addr.first;
		const unsigned char core = rand() % system_struct_t::NUM_OF_CPUS;
		const migration_cell_t mc(tid, core, page_t.pid, true);

		ret.push_back(mc);
		return ret;
	}
};

#endif
