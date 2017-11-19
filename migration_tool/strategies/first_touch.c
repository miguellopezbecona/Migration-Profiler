#include "first_touch.h"

// Migrates pages if they were accessed by a thread residing in a different memory node
vector<migration_cell_t> first_touch_t::get_pages_to_migrate(page_table_t *page_t){
	vector<migration_cell_t> ret;

	// Loops over all the pages
	for(auto const & t_it : page_t->page_node_map) {
		long int addr = t_it.first;
		pg_perf_data_t pd = t_it.second;

		// Gets memory node of last CPU access
		int cpu_node = system_struct_t::get_cpu_memory_cell(pd.last_cpu_access);

		// Compares to page location and adds to migration list if different
		int page_node = pd.current_node;
		if(cpu_node != page_node){
			migration_cell_t mc(addr, cpu_node, page_t->pid, false);
			ret.push_back(mc);
		}
	}

	return ret;
}

