#include "migration_algorithm.h"

#include "../strategies/random.h"

int total_thread_migrations = 0;
int total_page_migrations = 0;

int perform_migration_strategy(page_table_t *page_t){
	// Let's asume we do a thread and a page migration each iteration

	// Right now, we will use a random strategy
	random_t rand_st;

	migration_cell_t th_migr = rand_st.get_thread_to_migrate(page_t);
	th_migr.perform_thread_migration();

	migration_cell_t page_migr = rand_st.get_page_to_migrate(page_t);
	page_migr.perform_page_migration(page_t->pid);
}

