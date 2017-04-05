#include "migration_algorithm.h"

#include "../strategies/random.h"
#include "../strategies/genetic.h"

int total_thread_migrations = 0;
int total_page_migrations = 0;

// It will be probably discarded in the future
int perform_migration_strategy(page_table_t *page_t){
	// Let's asume we do a thread and a page migration each iteration

	// Right now, we can use a random strategy
	random_t rand_st;

	vector<migration_cell_t> ths_migr = rand_st.get_threads_to_migrate(page_t);
	for(migration_cell_t const & thm : ths_migr)
		thm.perform_thread_migration();

	vector<migration_cell_t> pages_migr = rand_st.get_pages_to_migrate(page_t);
	for(migration_cell_t const & pgm : pages_migr)
		pgm.perform_page_migration();

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

int perform_migration_strategy(map<pid_t, page_table_t> *page_ts){
	// Initial version of a genetic
	genetic_t gen_st;
	vector<migration_cell_t> pages_gen = gen_st.get_pages_to_migrate(page_ts);

	// Performs generated migrations. It must handle possible errors
	for(migration_cell_t const & pgm : pages_gen){
		int ret = pgm.perform_migration();
		
		if(ret != 0){ // There was an error
			handle_error(ret, pgm, page_ts);

			// Instead of aborting the iteration, we could just remove the following migration cells concerning that PID/TID. Right now, for simplicity:
			return ret;
		}
	}

	return 0;
}

