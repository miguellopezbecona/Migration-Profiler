#include "thread_migration.h"

memory_data_list_t memory_data_list;
inst_data_list_t inst_data_list;
pid_list_t pid_map; // Includes TID map
//page_list_t main_page_list;
page_table_t main_page_table;

unsigned int step=0;
time_t last_process;
int act_th_mig = ACTIVATE_THREAD_MIGRATION;
int act_pag_mig = ACTIVATE_PAGE_MIGRATION; 

int do_migration(pid_t pid);


int do_migration_and_clear_temp_list(pid_t pid, int do_thread_migration, int do_page_migration){
	act_th_mig = do_thread_migration;
	act_pag_mig = do_page_migration;
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
	if(sample.dsrc!=0)
		memory_data_list.add_cell(sample.cpu,sample.pid,sample.tid,sample.sample_addr,sample.weight,sample.dsrc,sample.time);
	return 1;
}

int process_inst_sample(my_pebs_sample_t sample){
	inst_data_list.add_cell(sample.cpu,sample.pid,sample.tid,sample.values[1],sample.values[0],sample.time);
	return 1;
}

//For use with no output
int process_my_pebs_sample(pid_t pid, my_pebs_sample_t sample){
	int ret;
	if(is_addr_sample(sample)){
		ret=process_addr_sample(sample);
	}else{
		ret=process_inst_sample(sample);
	}
	return ret;
}

int do_migration(pid_t pid){
	int ret;
	//printf("\nPrinting memory list...\n");
	//memory_data_list.print();

	//printf("\nPrinting instructions list...\n");
	//inst_data_list.print();

	ret = inst_data_list.create_increments();
	if(ret==-1){
		printf("No instruction data needed for migration\n");
		return RET_NO_INST;
	}
	//printf("increments created\n");
	//inst_data_list.print();
	rooflines(step, &memory_data_list, inst_data_list, &pid_map);
	//pid_map.print_pid_list();
	//pid_map.print_pid_list_redux();

	
	// Migrates threads and/or pages	
	if(step>0){
		if(act_th_mig)
			migrateThreads(pid, &pid_map, step);

		if(act_pag_mig) {
			//builds pages table
			pages(step, memory_data_list, &main_page_table);
			migratePages(pid, &main_page_table);
		}
	}else{
		init_migration_algorithm();
		core_table.print();
	}
	
	step++;
	return 0;
}
