#include "random.h"

// Picks random page and destination
migration_cell_t random_t::get_page_to_migrate(page_table_t *page_t){
	migration_cell_t ret;

	set<long int> s = page_t->uniq_addrs;
	int pos = rand() % s.size();
	set<long int>::iterator it = s.begin();
	for (; pos != 0; pos--) it++;

	ret.elem.page_addr = *it;
	ret.dest.mem_node = rand() % SYS_NUM_OF_MEMORIES;

	return ret;
}

// Picks random thread and destination
migration_cell_t random_t::get_thread_to_migrate(pid_t pid, page_table_t *page_t){
	migration_cell_t ret;

	ret.elem.tid = pid + (rand() % SYS_NUM_OF_CORES);
	ret.dest.core = rand() % SYS_NUM_OF_CORES;

	return ret;
}

