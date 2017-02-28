#include "migration_facade.h"

memory_data_list_t memory_data_list;
inst_data_list_t inst_data_list;
map<pid_t, page_table_t> page_tables; // Each table is associated to a specific PID

unsigned int step = 0;
bool action = false;

//>0 if addr_sample, 0 if inst_sample
bool is_addr_sample(my_pebs_sample_t sample){
	return sample.nr == 1;
}

int process_addr_sample(my_pebs_sample_t sample){
	if(sample.dsrc != 0)
		memory_data_list.add_cell(sample.cpu,sample.pid,sample.tid,sample.sample_addr,sample.weight,sample.dsrc,sample.time);
	return 1;
}

int process_inst_sample(my_pebs_sample_t sample){
	inst_data_list.add_cell(sample.cpu,sample.pid,sample.tid,sample.values[1],sample.values[0],sample.time);
	return 1;
}

// For use with no output
int process_my_pebs_sample(my_pebs_sample_t sample){
	if(is_addr_sample(sample))
		return process_addr_sample(sample);
	else
		return process_inst_sample(sample);
}

int begin_migration_process(vector<pid_t> pids, int do_thread_migration, int do_page_migration){

	//printf("\nPrinting memory list...\n");
	//memory_data_list.print();

	//printf("\nPrinting instructions list...\n");
	//inst_data_list.print();

	// Let's take out the data we are not interested in
	memory_data_list.filter_by_pids(pids);

	inst_data_list.create_increments();
	//printf("Printing inst list after creating increments...\n");
	//inst_data_list.print();

	// Builds pages tables for each PID
	pages(step, pids, memory_data_list, &page_tables);

	// This can be uncommented to alternate between migrating pages and threads each iteration
	/*
	action = !action;
	switch(action){
		case ACTIVATE_THREAD_MIGRATION:
			migrate_only_threads(...);
			break;
		case ACTIVATE_PAGE_MIGRATION:
			migrate_only_pages(...);
			break;
	}
	*/

	// Migrates threads and/or pages
	for (map<pid_t, page_table_t>::iterator it = page_tables.begin(); it != page_tables.end(); ++it){
		printf("Performing migration strategy for PID: %d\n", it->first);
		perform_migration_strategy(it->first, &it->second);
		//it->second.print();
	}
	
	step++;

	//printf("Mem list size: %lu, inst list size: %lu\n", memory_data_list.list.size(), inst_data_list.list.size());
	memory_data_list.clear();
	inst_data_list.clear();

	return 0;
}
