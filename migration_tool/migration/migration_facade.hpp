#pragma once
#ifndef __MIGRATION_FACADE__
#define __MIGRATION_FACADE__

#include <iostream>

#include <vector>
#include <map>
#include <set>

#include "sample_data.hpp"
#include "memory_list.hpp"
#include "inst_list.hpp"
#include "pages_ops.hpp"
#include "migration_algorithm.hpp"

// How many samples do you want to log in the CSV files in JUST_PROFILE mode?
const int ELEMS_PER_PRINT = 1000000;

#define DO_MIGRATIONS

// Macros for some specific analysis
//#define MEAN_ACS_ANALY
//#define MEAN_LAT_ANALY

#ifdef JUST_PROFILE // For dumping every sample in a mixed way
std::vector<my_pebs_sample_t> samples;
std::map<pid_t, std::vector<pid_t>> pmap;
#else // For building migration strategy
memory_data_list_t memory_list;
inst_data_list_t inst_list;
std::set<pid_t> pids;

std::map<pid_t, page_table_t> page_tables; // Keeps everything
std::map<pid_t, page_table_t> temp_page_tables; // Maintains data from current iteration
#endif

unsigned int step = 0;

void add_data_to_list (const my_pebs_sample_t & sample) {
	#ifdef JUST_PROFILE // Just adds samples to list
	samples.push_back(sample);

	// Gets children processes por sample's PID
	pid_t pid = sample.pid;
	if (!pmap.count(pid)) // !contains(pid)
		pmap[pid] = get_children_processes(pid);

	// Time to do a periodic print!
	if (samples.size() == ELEMS_PER_PRINT) {
		print_everything(samples, pmap);

		samples.clear();
		pmap.clear();
	}
	#else // Separates memory and inst samples
	if (sample.is_mem_sample()) { // [TOTHINK]: Before, we discarded samples with DSRC == 0, why?
		memory_list.add_cell(sample.cpu,sample.pid,sample.tid,sample.sample_addr,sample.weight,sample.dsrc,sample.time);
		//memory_list.list.back().print_dsrc(); printf("\n"); // Temporal, for testing dsrc printing
		pids.insert(sample.pid); // We will consider a process is active if we get a memory sample from it
	} else
		inst_list.add_cell(sample.cpu,sample.pid,sample.tid,sample.values[1],sample.values[0],sample.time);
	#endif
}

#ifdef JUST_PROFILE_ENERGY
void add_energy_data_to_last_sample () {
	//if(!samples.empty()) // Not necessary, but maybe one could want sanity checking
	samples.back().add_energy_data();
}
#endif

void clean_migration_structures () {
	#ifdef JUST_PROFILE
	// Prints remaining samples
	print_everything(samples, pmap);

	samples.clear();
	pmap.clear();
	#else
	memory_list.clear();
	inst_list.clear();
	pids.clear();

	// Final print for a specific analysis: PID, mean accesses and amount of pages
	#ifdef MEAN_ACS_ANALY
	std::cout << "pid,mean_acs,page_count" << '\n';
	const auto precision << std::cout.precision();
	std::cout.precision(3);
	for (auto const & it : page_tables) {
		page_table_t t = it.second;
		size_t sz = t.uniq_addrs.size();
		double mean = t.get_mean_acs_to_pages();
		//if(sz > 10 && mean > 1.25) // Trying to print only wanted PID
		std::cout << it.first << "," << mean << "," << sz << '\n';
	}
	std::cout << '\n';
	#endif

	// Final print for a specific analysis: PID, mean latency
	#ifdef MEAN_LAT_ANALY
	std::cout << "pid,mean_lat" << '\n';
	for(auto const & it : page_tables){
		page_table_t t = it.second;
		std::cout << it.first << "," << t.get_mean_lat_to_pages() << '\n';
	}
	std::cout.precision(precision);
	#endif

	page_tables.clear();
	#endif
}

#ifdef FAKE_DATA
// For testing purposes, specially on a non-native Linux system
void work_with_fake_data () {
	// Fake data from samples to be inserted into tables and etc
	memory_list.add_cell(0,500,500,0x12345000,10,0,0);
	memory_list.add_cell(0,500,500,0x12345000,100,0,1000);
	memory_list.add_cell(0,500,501,0x12345000,500,0,2000);
	inst_list.add_cell(0,500,500,50000000L,1000,0);
	inst_list.add_cell(0,500,500,50000000L,500,1000);
	inst_list.add_cell(0,500,501,50000000L,1500,1000);
	inst_list.add_cell(0,500,501,50000000L,100,2000);
	memory_list.add_cell(0,500,502,0x12345000,500,0,2000);

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
	pages(pids, memory_list, inst_list, &page_tables);

	// Creates/updates TID -> PID associations
	for (auto const & it : page_tables) {
		pid_t pid = it.first;
		page_table t = it.second;
		for (pid_t const & tid : t.get_tids())
			system_struct_t::set_pid_to_tid(pid, tid);
	}

	//tid_cpu_table.print(); // Debugging purposes

	// For testing some iterations of genetic
	for (int i = 0; i < 5; i++) {
		perform_migration_strategy(&page_tables);
	}

	//perform_migration_strategy(&page_tables[500]);

	clean_migration_structures();
	system_struct_t::clean();
}
#endif

#ifndef JUST_PROFILE
int begin_migration_process () {
	if (memory_list.is_empty()) {
		//printf("Memory list is empty. Skipping iteration...\n");
		return -1;
	}

	//printf("\nPrinting memory list...\n");
	//memory_list.print();

	//printf("\nPrinting instructions list...\n");
	//inst_list.print();

	#ifdef USE_ANNEA_ST	// Useless if we don't use annealing strategy
	inst_list.create_increments();
	//printf("Printing inst list after creating increments...\n");
	//inst_list.print();
	#endif

	// Builds page tables for each PID
	// pages(pids, memory_list, inst_list, &page_tables); // For historic data
	pages(pids, memory_list, inst_list, temp_page_tables);

	// For each existent table, checks if its associated process and/or threads are alive, to clean useless data
	// It also applies a single-PID migration strategy for active PIDs (i.e: we got at least a memory sample from it in this iteration)
	for (auto t_it = temp_page_tables.begin(); t_it != temp_page_tables.end(); ) {
		pid_t pid = t_it->first;
		page_table_t & table = t_it->second;
		//printf("Working over table associated to PID: %d\n", pid);

		// Is PID alive?
		if (!is_pid_alive(pid)) {

			// We get TIDs from the dead PID so we can remove those rows from tid_cpu_table and system_struct
			for (pid_t const & tid : table.get_tids()) {
				tid_cpu_table.remove_row(tid);
				system_struct_t::remove_tid(tid, false);
			}

			t_it = temp_page_tables.erase(t_it); // We erase entry from table map
			continue;
		}
		t_it++; // For erasing correctly

		// We check finished TIDs and remove them from table
		#ifdef USE_ANNEA_ST	// Since we can't check in page_table.c if this macro is defined, we have to do a little trick to avoid a bug
		table.remove_finished_tids(true);
		#else
		table.remove_finished_tids(false);
		#endif

		// Creates/updates TID -> PID associations
		for (pid_t const & tid : table.get_tids())
			system_struct_t::set_pid_to_tid(pid, tid);

		//table->print(); // For debugging

		#ifdef DO_MIGRATIONS
		if (pids.count(pid)) // contains(pid)
			perform_migration_strategy(table);
		#endif
	}

	#ifdef DO_MIGRATIONS
	//printf("Going to perform the global strategy.\n");
	//perform_migration_strategy(&page_tables);
	perform_migration_strategy(temp_page_tables); // Over tables from current iteration instead

	step++;
	#endif

	//printf("Mem list size: %lu, inst list size: %lu\n", memory_list.list.size(), inst_list.list.size());
	memory_list.clear();
	inst_list.clear();
	pids.clear(); // To maintain only active PIDs in each iteration

	temp_page_tables.clear(); // Only-current-iteration data is erased

	return 0;
}
#endif

#endif
