#include "migration_algorithm.h"

#include "../strategies/annealing.h"
#include "../strategies/first_touch.h"
#include "../strategies/genetic.h"
#include "../strategies/random.h"

// Strategies PID-independent
int perform_migration_strategy(page_table_t *page_t){
	// You can mix strategies at your wish

	vector<migration_cell_t> ths_migr;
	vector<migration_cell_t> pages_migr;

	#ifdef USE_RAND_ST
	random_t rand_st;
	ths_migr = rand_st.get_threads_to_migrate(page_t);
	for(migration_cell_t const & thm : ths_migr)
		thm.perform_thread_migration();

	pages_migr = rand_st.get_pages_to_migrate(page_t);
	for(migration_cell_t const & pgm : pages_migr)
		pgm.perform_page_migration();
	page_t->update_page_locations(pages_migr);
	#endif

	// First touch policy for pages
	#ifdef USE_FIRST_ST
	first_touch_t ft_st;
	pages_migr = ft_st.get_pages_to_migrate(page_t);
	for(migration_cell_t const & pgm : pages_migr)
		pgm.perform_page_migration();
	page_t->update_page_locations(pages_migr);
	#endif

	// Annealing strategy for threads, should be better as global strategy, but it didn't seem so...
	#ifdef USE_ANNEA_ST
	annealing_t a_st;
	ths_migr = a_st.get_threads_to_migrate(page_t);
	for(migration_cell_t const & thm : ths_migr)
		thm.perform_thread_migration();
	#endif

	return 0;
}

void handle_error(int errn, migration_cell_t mc, map<pid_t, page_table_t> *page_ts) {
	switch(errn){
		case ESRCH: // Non-existing PID/TID. It has to be removed
			if(mc.is_thread_cell()){
				printf("Removing inexistent TID %lu...\n", mc.elem);
				page_ts->operator[](mc.pid).remove_tid(mc.elem);
			} else {
				printf("Removing inexistent PID %d...\n", mc.pid);
				page_ts->erase(mc.pid);
			}

			break;
		default: // More possible error handling
			printf("Error code while migrating: %d\n", errn);
			printf("Failed migration cell's data:\n");
			mc.print();
	}
}

#ifdef USE_GEN_ST
genetic_t gen_st;
#endif

// All-system level strategies
int perform_migration_strategy(map<pid_t, page_table_t> *page_ts){
	int final_ret = 0;
	vector<migration_cell_t> ths_migr;
	vector<migration_cell_t> pgs_migr;

	// Very initial version of a genetic approach
	#ifdef USE_GEN_ST
	ths_migr = gen_st.get_threads_to_migrate(page_ts);

	// Performs generated migrations. It must handle possible errors
	for(migration_cell_t const & thm : ths_migr){
		int ret = thm.perform_thread_migration();
		
		if(ret != 0){ // There was an error
			handle_error(ret, thm, page_ts);
			final_ret += ret;
		}
	}
	#endif

	// Annealing strategy for threads. Does not work very well in this global version
/*
	#ifdef USE_ANNEA_ST
	annealing_t a_st;
	vector<migration_cell_t> ths_migr = a_st.get_threads_to_migrate(page_ts);
	for(migration_cell_t const & thm : ths_migr)
		thm.perform_thread_migration();
	#endif
*/

	return final_ret;
}

