#include "thread_migration.h"

memory_data_list_t memory_data_list;
inst_data_list_t inst_data_list;
page_table_t main_page_table; // Associated to a specific PID

unsigned int step = 0;
bool action = false;

int do_migration(pid_t pid);


int do_migration_and_clear_temp_list(pid_t pid, int do_thread_migration, int do_page_migration){
	do_migration(pid);
	//printf("mem %lu, inst %lu\n", memory_data_list.list.size(), inst_data_list.list.size());
	memory_data_list.clear();
	inst_data_list.clear();
}	

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
int process_my_pebs_sample(pid_t pid, my_pebs_sample_t sample){
	if(is_addr_sample(sample))
		return process_addr_sample(sample);
	else
		return process_inst_sample(sample);
}

int do_migration(pid_t pid){
	//printf("\nPrinting memory list...\n");
	//memory_data_list.print();

	//printf("\nPrinting instructions list...\n");
	//inst_data_list.print();

	int ret = inst_data_list.create_increments();
	if(ret==-1){
		printf("No instruction data needed for migration\n");
		return RET_NO_INST;
	}
	//printf("increments created\n");
	//inst_data_list.print();

	// Builds pages table
	pages(step, pid, memory_data_list, &main_page_table);

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
	perform_migration_strategy(pid, &main_page_table);
	
	step++;
	return 0;
}
