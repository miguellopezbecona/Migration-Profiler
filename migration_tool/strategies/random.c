#include "random.h"

// Picks random page and destination
vector<migration_cell_t> random_t::get_pages_to_migrate(page_table_t *page_t){
	vector<migration_cell_t> ret;
	migration_cell_t mc;

	// Searches random page address in set
	set<long int> s = page_t->uniq_addrs;
	int pos = rand() % s.size();
	set<long int>::iterator it = s.begin();
	for (; pos != 0; pos--) it++;

	mc.elem.page_addr = *it;
	mc.dest.mem_node = rand() % SYS_NUM_OF_MEMORIES;

	ret.push_back(mc);
	return ret;
}

// Picks random thread and destination
vector<migration_cell_t> random_t::get_threads_to_migrate(page_table_t *page_t){
	vector<migration_cell_t> ret;
	migration_cell_t mc;

	// Searches random TID in map
	map<int, short> m = page_t->tid_index;
	int pos = rand() % m.size();
	map<int, short>::iterator it = m.begin();
	for (; pos != 0; pos--) it++;

	mc.elem.tid = it->first;
	mc.dest.core = rand() % SYS_NUM_OF_CORES;

	ret.push_back(mc);
	return ret;
}

