#pragma once
#ifndef __MIGRATION_ALGORITHM__
#define __MIGRATION_ALGORITHM__

#include <iostream>

// Uses page table
#include "page_table.hpp"

#if defined(USE_ANNEA_ST)
#include "strategies/annealing.hpp"
#elif defined(USE_RAND_ST)
#include "strategies/random.hpp"
#elif defined(USE_GEN_ST)
#include "strategies/genetic.hpp"
#elif defined(USE_FIRST_ST)
#include "strategies/first_touch.hpp"
#elif defined(USE_ENER_ST)
#include "strategies/energy.hpp"
#endif

extern size_t step; // Iteration number

int perform_migration_strategy (page_table_t & page_t) { // For a one process-only strategy
	// You can mix strategies at your wish

	std::vector<migration_cell_t> ths_migr;
	std::vector<migration_cell_t> pgs_migr;

	#ifdef USE_RAND_ST
	random_t rand_st;
	ths_migr = rand_st.get_threads_to_migrate(page_t);
	for (migration_cell_t const & thm : ths_migr)
		thm.perform_thread_migration();

	pgs_migr = rand_st.get_pages_to_migrate(page_t);
	for (migration_cell_t const & pgm : pgs_migr)
		pgm.perform_page_migration();

	page_t.update_page_locations(pgs_migr);
	#endif

	// First touch policy for pages
	#ifdef USE_FIRST_ST
	first_touch_t ft_st;
	pgs_migr = ft_st.get_pages_to_migrate(page_t);
	for (migration_cell_t const & pgm : pgs_migr)
		pgm.perform_page_migration();

	page_t.update_page_locations(pgs_migr);
	#endif

	// Annealing strategy for threads, should be better as global strategy, but it didn't seem so...
/*
	#ifdef USE_ANNEA_ST
	annealing_t a_st;
	ths_migr = a_st.get_threads_to_migrate(page_t);
	for(migration_cell_t const & thm : ths_migr)
		thm.perform_thread_migration();
	#endif

*/
	return 0;
}

void handle_error (const int errn, const migration_cell_t mc, std::map<pid_t, page_table_t> & page_ts) {
	switch (errn) {
		case ESRCH: // Non-existing PID/TID. It has to be removed
			if (mc.is_thread_cell()) {
				std::cout << "Removing inexistent TID " << mc.elem << "..." << '\n';
				page_ts[mc.pid].remove_tid(mc.elem);
			} else {
				std::cout << "Removing inexistent PID " << mc.pid << "..." << '\n';
				page_ts.erase(mc.pid);
			}

			break;
		default: // More possible error handling
			std::cerr << "Error code while migrating: " << errn << '\n';
			std::cerr << "Failed migration cell's data:" << '\n';
			std::cerr << mc << '\n';
	}
}

#ifdef USE_GEN_ST
genetic_t gen_st;
#endif

int perform_migration_strategy (std::map<pid_t, page_table_t> & page_ts) { // For a global strategy
	int final_ret = 0;
	std::vector<migration_cell_t> ths_migr;
	std::vector<migration_cell_t> pgs_migr;

	// Very initial version of a genetic approach
	#ifdef USE_GEN_ST
	ths_migr = gen_st.get_threads_to_migrate(page_ts);

	// Performs generated migrations. It must handle possible errors
	for (migration_cell_t const & thm : ths_migr) {
		int ret = thm.perform_thread_migration();

		if (ret != 0) { // There was an error
			handle_error(ret, thm, page_ts);
			final_ret += ret;
		}
	}
	#endif

	// Annealing strategy for threads. Does not work very well in this global version

	#ifdef USE_ANNEA_ST
	annealing_t a_st;
	ths_migr = a_st.get_threads_to_migrate(page_ts);
	for (migration_cell_t const & thm : ths_migr)
		thm.perform_thread_migration();
	#endif

	#ifdef USE_ENER_ST
	energy_str_t e_st;
	ths_migr = e_st.get_threads_to_migrate(page_ts);
	for (migration_cell_t const & thm : ths_migr)
		thm.perform_thread_migration();

/*
	pgs_migr = e_st.get_pages_to_migrate(page_ts);
	for(migration_cell_t const & pgm : pgs_migr){
		pgm.perform_page_migration();
		page_ts->operator[](pgm.pid).update_page_location(pgm);
	}
*/
	#endif

	return final_ret;
}

#endif
