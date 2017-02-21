#include "migration_algorithm.h"

#include "migration_cell.h"

int total_thread_migrations = 0;
int total_page_migrations = 0;

// Core function to select the page to migrate and where. Random right now
migration_cell_t get_page_to_migrate(page_table_t *page_t){
	migration_cell_t ret;

	set<long int> s = page_t->uniq_addrs;
	int pos = rand() % s.size();
	set<long int>::iterator it = s.begin();
	for (; pos != 0; pos--) it++;

	ret.elem.page_addr = *it;
	ret.dest.mem_node = rand() % SYS_NUM_OF_MEMORIES;

	return ret;
}

// Core function to select the thread to migrate and where. Random right now
migration_cell_t get_thread_to_migrate(pid_t pid, page_table_t *page_t){
	migration_cell_t ret;

	ret.elem.tid = pid + (rand() % SYS_NUM_OF_CORES); // Should get TID from a list...
	ret.dest.core = rand() % SYS_NUM_OF_CORES;

	return ret;
}


int perform_migration_strategy(pid_t pid, page_table_t *page_t){
	// Let's asume we do a thread and a page migration each iteration
	migration_cell_t th_migr = get_thread_to_migrate(pid, page_t);
	th_migr.perform_thread_migration();

	migration_cell_t page_migr = get_page_to_migrate(page_t);
	page_migr.perform_page_migration(pid);
}

