#pragma once
#ifndef __PAGES_OPS__
#define __PAGES_OPS__

#include <iostream>
#include <fstream>
#include <cmath>

#include <unistd.h>
// For move_pages, compile with -lnuma
#include <numa.h>
#include <numaif.h>

// Uses some structures
#include "memory_list.hpp"
#include "inst_list.hpp"
#include "page_table.hpp"

#include "migration_algorithm.hpp" // For checking strategy macros

//#define PERFORMANCE_OUTPUT

//#define PRINT_HEATMAPS
const int ITERATIONS_PER_HEATMAP_PRINT = 5;
const int NUM_FILES = 5;

#ifdef PRINT_HEATMAPS
// FILE *fps[NUM_FILES];
std::array<std::iofstream, NUM_FILES> fps;
#endif

const long pagesize = sysconf(_SC_PAGESIZE);
const int expn = log2(pagesize);

inline int get_page_current_node (const pid_t pid, const long int pageAddr) {
	void * pages[] = {(void *) pageAddr};
	int status;

	//printf("Getting page current node for page addr 0x%016lx for PID %d. It is: ", pageAddr, pid);

	// If "nodes" parameter is NULL, in status we obtain the node where the page is
	numa_move_pages(pid, 1, pages, NULL, &status, MPOL_MF_MOVE);

	//printf("%d\n",status);

	return status;
}

void build_page_tables (const memory_data_list_t & m_list, const inst_data_list_t & i_list, std::map<pid_t, page_table_t> & page_ts) {
	// Builds data from m_list, but first we calculate page address, page node, and then it is added to the table cell
	for (memory_data_cell_t const & m_cell : m_list.list) {

		// This gets page address with offset 0
		long int page_addr = m_cell.addr & ~((long int)(pagesize - 1));

		// This gets page number
		//long int page_num = m_cell.addr >> expn;

		const auto page_node = get_page_current_node(m_cell.tid, page_addr);
		const auto cpu_node = system_struct_t::get_cpu_memory_node(m_cell.cpu);

		// We filter pages that give problems
		if (page_node < 0)
			continue;

		// Registers TID in system structure if needed
		system_struct_t::add_tid(m_cell.tid, m_cell.cpu);

		if (!page_ts.count(m_cell.pid)) { // = !contains(pid). We init the entry if it doesn't exist
			page_ts[m_cell.pid] = page_table_t(m_cell.pid);
		}

		page_ts[m_cell.pid].add_cell(page_addr, page_node, m_cell.tid, m_cell.latency, m_cell.cpu, cpu_node, m_cell.is_cache_miss());

		// [EXPERIMENTAL]: we update TID/cpu perf_table
		tid_cpu_table.add_data(m_cell.tid, m_cell.cpu, m_cell.latency);
	}

	#ifdef USE_ANNEA_ST	// Unnecessary processing if we don't use annealing strategy
	// Adds inst data from i_list for better performance calculation
	for (inst_data_cell_t const & i_cell : i_list.list) {

		// If there is no table associated (no mem sample) to the pid, the data will be discarded
		if (!page_ts.count(i_cell.pid)) // = !contains(pid)
			continue;

		page_ts[i_cell.pid].add_inst_data_for_tid(i_cell.tid, i_cell.cpu, i_cell.inst, i_cell.req_dr, i_cell.time);
	}

	// After all the sums from insts are done, the perf is calculated
	for (auto const & it : page_ts)
		page_ts[it.first].calc_perf();
	#endif

	#ifdef PERFORMANCE_OUTPUT
	// This generates way too much output. Uncomment only when really necessary
/*
	for(auto const & it : *page_ts)
		it.second.print_performance();
*/
	tid_cpu_table.print();
	#endif
}

int pages (const std::set<pid_t> & pids, const memory_data_list_t & m_list, const inst_data_list_t & i_list, std::map<pid_t, page_table_t> & page_ts) {
	// Builds tables
	build_page_tables(m_list, i_list, page_ts);

	// Some optional things are done every ITERATIONS_PER_HEATMAP_PRINT iterations
	#ifdef PRINT_HEATMAPS
	if (step % ITERATIONS_PER_HEATMAP_PRINT == 0) {
		for (pid_t const & pid : pids) {
			page_table_t & t = page_ts[pid];

			const char* CSV_NAMES[] = {"acs", "min", "avg", "max", "alt"};

			// for (auto & file : fps) {
			for (size_t i = 0; i < NUM_FILES; i++) {
				char filename[32] = "\0";
				sprintf(filename, "%s_%d_%d.csv", CSV_NAMES[i], pid, current_step);

				auto & file = fps[i];
				file.open(filename);
				if (!file.is_open()) {
					std::cerr << "Error opening file " << filename << " to log heatmap." << '\n';
					return -1;
				}
			}

			t.print_heatmaps(fps, NUM_FILES-1);
			t.print_alt_graph(fps[NUM_FILES-1]);

			for (auto & file : fps) {
				file.close();
			}

			//t.calculate_performance_page(1000);
			//t.print_performance();
		}
	}
	#endif

	return 0;
}

#endif
