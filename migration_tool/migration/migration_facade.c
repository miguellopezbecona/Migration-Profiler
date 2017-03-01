#include "migration_facade.h"

memory_data_list_t memory_data_list;
inst_data_list_t inst_data_list;
vector<pid_t> pids;
map<pid_t, page_table_t> page_tables; // Each table is associated to a specific PID

unsigned int step = 0;
bool action = false;

bool is_addr_sample(my_pebs_sample_t sample){
	return sample.nr == 1;
}

void add_data_to_list(my_pebs_sample_t sample){
	pids.push_back(sample.pid);

	if(is_addr_sample(sample) && sample.dsrc != 0)
		memory_data_list.add_cell(sample.cpu,sample.pid,sample.tid,sample.sample_addr,sample.weight,sample.dsrc,sample.time);
	else
		inst_data_list.add_cell(sample.cpu,sample.pid,sample.tid,sample.values[1],sample.values[0],sample.time);
}

int begin_migration_process(int do_thread_migration, int do_page_migration){
	if(memory_data_list.is_empty()){
		//printf("Memory list is empty. Skipping iteration...\n");
		return -1;
	}

	//printf("\nPrinting memory list...\n");
	//memory_data_list.print();

	//printf("\nPrinting instructions list...\n");
	//inst_data_list.print();

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

	// Migrates threads and/or pages for active PIDs
	for (size_t i=0;i<pids.size();i++){
		pid_t pid = pids[i];
		page_table* table = &page_tables[pid];

		printf("Performing migration strategy for PID: %d\n", pid);
		perform_migration_strategy(table); 
		//table->print();
	}
	
	step++;

	//printf("Mem list size: %lu, inst list size: %lu\n", memory_data_list.list.size(), inst_data_list.list.size());
	memory_data_list.clear();
	inst_data_list.clear();
	pids.clear(); // To maintain only active PIDs in each iteration

	return 0;
}
