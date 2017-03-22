#include "migration_algorithm.h"

#include "../strategies/random.h"
#include "../strategies/genetic.h"

int total_thread_migrations = 0;
int total_page_migrations = 0;

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

int perform_migration_strategy(map<pid_t, page_table_t> *page_ts){
	// Initial version of a genetic
	genetic_t gen_st;
	vector<migration_cell_t> pages_gen = gen_st.get_pages_to_migrate(page_ts);
	for(migration_cell_t const & pgm : pages_gen)
		pgm.perform_migration();

	return 0;
}

