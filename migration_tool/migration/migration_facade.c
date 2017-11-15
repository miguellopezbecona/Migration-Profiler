#include "migration_facade.h"

memory_data_list_t memory_list;
inst_data_list_t inst_list;
set<pid_t> pids;
map<pid_t, page_table_t> page_tables; // Each table is associated to a specific PID

#ifdef JUST_PROFILE
vector<my_pebs_sample_t> samples;
map<pid_t, vector<pid_t>> pmap;
#endif

unsigned int step = 0;

void add_data_to_list(my_pebs_sample_t sample){
	#ifdef JUST_PROFILE
	samples.push_back(sample);

	// Gets children processes por sample's PID
	pid_t pid = sample.pid;
	if(pmap.count(pid) == 0) // !contains(pid)
		pmap[pid] = get_children_processes(pid);

	// Time to do a periodic print!
	if(samples.size() == ELEMS_PER_PRINT){
		print_everything(samples, pmap);

		samples.clear();
		pmap.clear();
	}
	#else
	if(sample.is_mem_sample() && sample.dsrc != 0){
		memory_list.add_cell(sample.cpu,sample.pid,sample.tid,sample.sample_addr,sample.weight,sample.dsrc,sample.time);
		//memory_list.list.back().print_dsrc(); printf("\n"); // Temporal, for testing dsrc printing
		pids.insert(sample.pid); // Currently we only use memory list so we only consider a thread is active if we get a memory sample
	} else
		inst_list.add_cell(sample.cpu,sample.pid,sample.tid,sample.values[1],sample.values[0],sample.time);
	#endif
}

void clean_migration_structures(){
	memory_list.clear();
	inst_list.clear();
	pids.clear();

	// Final print for a specific analysis: PID, mean accesses and amount of pages
/*
	for(auto & it : page_tables){
		page_table_t t = it.second;
		size_t sz = t.uniq_addrs.size();
		double mean = t.get_mean_acs_to_pages();
		//if(sz > 10 && mean > 1.25) // Trying to print only wanted PID
			printf("\t%d,%.2f,%lu\n", it.first, mean, sz);
	}
*/

	// Final print for a specific analysis: PID, mean latency
/*
	for(auto & it : page_tables){
		page_table_t t = it.second;
		double mean = t.get_mean_lat_to_pages();
		printf("%d,%.2f\n", it.first, mean);
	}
*/
	page_tables.clear();

	#ifdef JUST_PROFILE
	// Prints remaining samples
	print_everything(samples, pmap);

	samples.clear();
	pmap.clear();
	#endif
}

// For testing purposes, specially on a non-native Linux system
void work_with_fake_data(){
	// Fake data from samples to be inserted into tables and etc
	memory_list.add_cell(0,500,500,0x12345000,10,0,0);
	memory_list.add_cell(0,500,500,0x12345000,100,0,1000);
	memory_list.add_cell(0,500,501,0x12345000,500,0,2000);
	inst_list.add_cell(0,500,500,50000000L,1000,0);
	inst_list.add_cell(0,500,500,50000000L,500,1000);
	inst_list.add_cell(0,500,501,50000000L,1500,1000);
	inst_list.add_cell(0,500,501,50000000L,100,2000);

/*  // Fake data into table directly
	page_table_t t1(500);
	t1.add_cell(0x12345000, 0, 500, 10, 0, 0, false);
	t1.add_cell(0x12345000, 0, 500, 30, 0, 0, false);
	t1.add_cell(0x12345000, 0, 501, 5, 0, 0, false);
	t1.add_cell(0x12346000, 0, 500, 1, 0, 0, false);

	page_table_t t2(1000);
	t2.add_cell(0x12341000, 0, 1000, 5, 0, 0, false);
	t2.add_cell(0x12342000, 0, 1000, 100, 0, 0, false);
	t2.add_cell(0x12343000, 0, 1000, 200, 0, 0, false);

	page_tables[500] = t1;
	page_tables[1000] = t2;
*/
	pages(step, pids, memory_list, inst_list, &page_tables);
	pids.insert(500);

	for (pid_t const & pid : pids){		
		page_table* table = &page_tables[500];
		perform_migration_strategy(table);
	}

/*
	for(auto & it : page_tables){
		page_table_t t = it.second;
		t.print();
		t.print_performance();
		printf("Mean of the number of page accesses for that PID: %.2f\n\n", t.get_mean_acs_to_pages());
	}
*/
}

int begin_migration_process(int do_thread_migration, int do_page_migration){
	if(memory_list.is_empty()){
		//printf("Memory list is empty. Skipping iteration...\n");
		return -1;
	}

	//printf("\nPrinting memory list...\n");
	//memory_list.print();

	//printf("\nPrinting instructions list...\n");
	//inst_list.print();

	inst_list.create_increments();
	//printf("Printing inst list after creating increments...\n");
	//inst_list.print();

	// Builds pages tables for each PID
	pages(step, pids, memory_list, inst_list, &page_tables);

	// For each active PID, cleans "dead" TIDs from its table and it can perform a single-process migration strategy
	for (pid_t const & pid : pids){
		//printf("Working over table associated to PID: %d\n", pid);

		// Sanity checking. It can seem redundant but it is necessary!
		if(!is_pid_alive(pid)){
			printf("\tProcess dead even after getting sample data. Removing it from map...\n");
			page_tables.erase(pid);
			continue;
		}
		
		page_table* table = &page_tables[pid];
		table->remove_inactive_tids();
		//table->print();

		perform_migration_strategy(table);
	}
	//printf("Going to perform the global strategy.\n");
	//perform_migration_strategy(&page_tables);

	step++;

	//printf("Mem list size: %lu, inst list size: %lu\n", memory_list.list.size(), inst_list.list.size());
	memory_list.clear();
	inst_list.clear();
	pids.clear(); // To maintain only active PIDs in each iteration

	return 0;
}
