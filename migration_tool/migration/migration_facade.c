#include "migration_facade.h"

memory_data_list_t memory_data_list;
inst_data_list_t inst_data_list;
set<pid_t> pids;
map<pid_t, page_table_t> page_tables; // Each table is associated to a specific PID

unsigned int step = 0;

void add_data_to_list(my_pebs_sample_t sample){
	if(sample.is_addr_sample() && sample.dsrc != 0){
		memory_data_list.add_cell(sample.cpu,sample.pid,sample.tid,sample.sample_addr,sample.weight,sample.dsrc,sample.time);
		pids.insert(sample.pid); // Currently we only use memory list so we only consider a thread is active if we get a memory sample
	} else
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

	//inst_data_list.create_increments(); // Not necessary right now
	//printf("Printing inst list after creating increments...\n");
	//inst_data_list.print();

	// Builds pages tables for each PID
	pages(step, pids, memory_data_list, &page_tables);

	// For each active PID, cleans "dead" TIDs from its table and performs a migration strategy
	for (pid_t pid : pids){
		printf("Working over table associated to PID: %d\n", pid);

		page_table* table = &page_tables[pid];
		table->remove_inactive_tids();
		//table->print();

		//perform_migration_strategy(table); // Commented because we are going to perform a migration strategy globally
	}
	printf("Going to perform the global strategy.\n");
	perform_migration_strategy(&page_tables);
	
	step++;

	//printf("Mem list size: %lu, inst list size: %lu\n", memory_data_list.list.size(), inst_data_list.list.size());
	memory_data_list.clear();
	inst_data_list.clear();
	pids.clear(); // To maintain only active PIDs in each iteration

	return 0;
}
