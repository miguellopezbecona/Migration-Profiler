#include "pages_ops.h"

#include <math.h> // log2

#ifdef PRINT_HEATMAPS
FILE *fps[NUM_FILES];
#endif

const long pagesize = sysconf(_SC_PAGESIZE);
const int expn = log2(pagesize);

void build_page_tables(memory_data_list_t m_list, inst_data_list_t i_list, map<pid_t, page_table_t> *page_ts){
	// Builds data from m_list, but first we calculate page address, page node, and then it is added to the table cell
	for(memory_data_cell_t const & m_cell : m_list.list){

		// This gets page address with offset 0
		long int page_addr = m_cell.addr & ~((long int)(pagesize -1));

		// This gets page number
		//long int page_num = m_cell.addr >> expn;

		int page_node = get_page_current_node(m_cell.tid, page_addr);
		int cpu_node = system_struct_t::get_cpu_memory_node(m_cell.cpu);

		// We filter pages that give problems
		if(page_node < 0)
			continue;

		// Registers TID in system structure if needed
		system_struct_t::add_tid(m_cell.tid, m_cell.cpu);

		if(!page_ts->count(m_cell.pid)){ // = !contains(pid). We init the entry if it doesn't exist
			page_table_t t(m_cell.pid);
			page_ts->operator[](m_cell.pid) = t;
		}

		page_ts->operator[](m_cell.pid).add_cell(page_addr, page_node, m_cell.tid, m_cell.latency, m_cell.cpu, cpu_node, m_cell.is_cache_miss());

		// [EXPERIMENTAL]: we update TID/cpu perf_table
		//tid_cpu_table.add_data(m_cell.tid, m_cell.cpu, m_cell.latency);
	}

	#ifdef USE_ANNEA_ST	// Unnecessary processing if we don't use annealing strategy
	// Adds inst data from i_list for better performance calculation
	for(inst_data_cell_t const & i_cell : i_list.list){

		// If there is no table associated (no mem sample) to the pid, the data will be discarded
		if(!page_ts->count(i_cell.pid)) // = !contains(pid)
			continue;
		
		page_ts->operator[](i_cell.pid).add_inst_data_for_tid(i_cell.tid, i_cell.cpu, i_cell.inst, i_cell.req_dr, i_cell.time);
	}

	// After all the sums from insts are done, the perf is calculated
	for(auto const & it : *page_ts)
		page_ts->operator[](it.first).calc_perf();
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

inline int get_page_current_node(pid_t pid, long int pageAddr){
	void **pages = (void **)calloc(1,sizeof(long int *));
	pages[0] = (void *) pageAddr;
	int status;

	//printf("Getting page current node for page addr 0x%016lx for PID %d. It is: ", pageAddr, pid);

	#ifndef FAKE_DATA
	// If "nodes" parameter is NULL, in status we obtain the node where the page is
	numa_move_pages(pid, 1, pages, NULL, &status, MPOL_MF_MOVE);
	#endif

	//printf("%d\n",status);

	free(pages);

	return status;
}

int pages(set<pid_t> pids, memory_data_list_t m_list, inst_data_list i_list, map<pid_t, page_table_t> *page_ts){
	// Builds tables
	build_page_tables(m_list, i_list, page_ts);

	// Some optional things are done every ITERATIONS_PER_HEATMAP_PRINT iterations
	if(step % ITERATIONS_PER_HEATMAP_PRINT == 0){
		for (pid_t const & pid : pids){
			page_table_t t = page_ts->operator[](pid);

			#ifdef PRINT_HEATMAPS
			const char* CSV_NAMES[] = {"acs", "min", "avg", "max", "alt"};

			// Creates CSV files
			for(int i=0;i<NUM_FILES;i++){
				char filename[32] = "\0";
				sprintf(filename, "%s_%d_%d.csv", CSV_NAMES[i], pid, current_step);
				fps[i] = fopen(filename, "w");

				if(fps[i] == NULL){
					printf("Error opening file %s to log heatmap.\n", filename);
					return -1;
				}
			}

			t.print_heatmaps(fps, NUM_FILES-1);
			t.print_alt_graph(fps[NUM_FILES-1]);

			// Closes files
			for(int i=0;i<NUM_FILES;i++)
				fclose(fps[i]);
			#endif

			//t.calculate_performance_page(1000);
			//t.print_performance();
		}

	}

	return 0;
}

