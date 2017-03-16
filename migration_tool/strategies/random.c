#include "random.h"

// Picks random page and destination
vector<migration_cell_t> random_t::get_pages_to_migrate(page_table_t *page_t){
	vector<migration_cell_t> ret;

	// Searches random page address in set
	set<long int> s = page_t->uniq_addrs;
	int pos = rand() % s.size();
	set<long int>::iterator it = s.begin();
	for (; pos != 0; pos--) it++;

	// Creates migration cell with data
	long int page_addr = *it;
	unsigned char mem_node = rand() % SYS_NUM_OF_MEMORIES;
	migration_cell_t mc(page_addr, mem_node);

	ret.push_back(mc);
	return ret;
}

// Picks random thread and destination
vector<migration_cell_t> random_t::get_threads_to_migrate(page_table_t *page_t){
	vector<migration_cell_t> ret;

	// Searches random TID in map
	map<int, short> m = page_t->tid_index;
	int pos = rand() % m.size();
	map<int, short>::iterator it = m.begin();
	for (; pos != 0; pos--) it++;

	// Creates migration cell with data
	long int tid = it->first;
	unsigned char core = rand() % SYS_NUM_OF_CORES;
	migration_cell_t mc(tid, core);

	ret.push_back(mc);
	return ret;
}

