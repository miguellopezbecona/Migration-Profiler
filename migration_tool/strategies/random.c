#include "random.h"

// Picks random page and destination
migration_cell_t random_t::get_page_to_migrate(page_table_t *page_t){
	migration_cell_t ret;

	// Searches random page address in set
	set<long int> s = page_t->uniq_addrs;
	int pos = rand() % s.size();
	set<long int>::iterator it = s.begin();
	for (; pos != 0; pos--) it++;

	ret.elem.page_addr = *it;
	ret.dest.mem_node = rand() % SYS_NUM_OF_MEMORIES;

	return ret;
}

// Picks random thread and destination
migration_cell_t random_t::get_thread_to_migrate(page_table_t *page_t){
	migration_cell_t ret;

	// Searches random TID in map
	map<int, short> m = page_t->tid_index;
	int pos = rand() % m.size();
	map<int, short>::iterator it = m.begin();
	for (; pos != 0; pos--) it++;

	ret.elem.tid = it->first;
	ret.dest.core = rand() % SYS_NUM_OF_CORES;

	return ret;
}

