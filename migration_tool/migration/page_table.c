#include "page_table.h"

/*** table_cell_t ***/
table_cell_t::table_cell(int latency, bool is_cache_miss){
	latencies.push_back(latency);
	cache_misses = (unsigned) is_cache_miss;
}

void table_cell_t::update(int latency, bool is_cache_miss){
	latencies.push_back(latency);
	cache_misses += (unsigned) is_cache_miss;
}

void table_cell_t::print(){
	printf("M_LAT %.2f, T_ACC %lu, CACH_MIS: %u\n", accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size(), latencies.size(), cache_misses);
}

/*** page_table_t ***/
int page_table_t::add_cell(long int page_addr, int current_node, pid_t tid, int latency, int cpu, int cpu_node, bool is_cache_miss){
	table_cell_t *cell = get_cell(page_addr, tid);

	if(cell == NULL) {
		table_cell_t aux(latency, is_cache_miss);
		uniq_addrs.insert(page_addr); // Adds address to the set, won't be inserted if already added by other threads

		// If TID does not exist in map, we associate a index to it
		if(tid_index.count(tid) == 0){
			tid_index[tid] = table_index;
			table_index++;
		}
		int pos = tid_index[tid];
		table[pos][page_addr] = aux;
	} else
		cell->update(latency, is_cache_miss);
	
	page_node_map[page_addr].current_place = current_node; // Creates/updates association in page node map

	return 0;
}

bool page_table_t::contains_addr(long int page_addr, int tid){
	if(tid_index.count(tid) == 0)
		return false;

	int pos = tid_index[tid];
	return table[pos].count(page_addr) > 0;
}

vector<int> page_table_t::get_latencies_from_cell(long int page_addr, int tid){
	if(contains_addr(page_addr,tid)){
		int pos = tid_index[tid];
		return table[pos][page_addr].latencies;
	} else {
		vector<int> v;
		return v;
	}
}


// Returns NULL if no association
table_cell_t* page_table_t::get_cell(long int page_addr, int tid){
	if(contains_addr(page_addr,tid)){
		int pos = tid_index[tid];
		return &table[pos][page_addr];
	} else
		return NULL;
}

int page_table_t::reset_column(long int page_addr, int current_node){
	for(map<int, short>::iterator it = tid_index.begin(); it != tid_index.end(); ++it) {
		table_cell_t *cell = get_cell(page_addr, it->first);
		if(cell != NULL)
			cell->latencies.clear();
	}
	
	page_node_map[page_addr].current_place = current_node;
	return 0;
}

void page_table_t::clear(){
	for(int i=0;i<SYS_NUM_OF_CORES;i++)
		table[i].clear();
}

void page_table_t::print(){
	printf("Page table for PID: %d\n", pid);

	for(map<int, short>::iterator t_it = tid_index.begin(); t_it != tid_index.end(); ++t_it) {
		int tid = t_it->first;
		int pos = t_it->second;

		printf("TID: %d\n", tid);
		if(table[pos].empty())
			printf("\tEmpty.\n");
		
		for(map<long int, table_cell_t>::iterator it = table[pos].begin(); it != table[pos].end(); ++it) {
			long int page_addr = it->first;
			table_cell_t cell = it->second;
			
			printf("\tPAGE_ADDR: %lx, CURRENT_NODE: %d, ", page_addr, page_node_map[page_addr].current_place);
			cell.print();
		}
		printf("\n");
	}
}

void page_table_t::print_heatmaps(FILE **fps, int num_fps){
	// This is for writing number of accesses by each thread in the last row
	int accesses[SYS_NUM_OF_CORES];
	memset(accesses, 0, sizeof(accesses));

	// First line of each CSV file: column header
	for(int i=0;i<num_fps;i++){
		// Rowname (first column)
		fprintf(fps[i], "addr,");

		// Threads' ID
		for(map<int, short>::iterator t_it = tid_index.begin(); t_it != tid_index.end(); ++t_it)
			fprintf(fps[i], "T-%d,", t_it->first);
		fprintf(fps[i], "\n");
	}

	// Each unique page address will write a row with the data for each thread
	for (set<long int>::iterator it = uniq_addrs.begin(); it != uniq_addrs.end(); ++it){
		long int addr = *it;

		// Writes address as row name (first column)
		for(int i=0;i<num_fps;i++)
			fprintf(fps[i], "%lx,", addr);

		// Writes data for each thread
		for(map<int, short>::iterator t_it = tid_index.begin(); t_it != tid_index.end(); ++t_it) {
			vector<int> l = get_latencies_from_cell(addr, t_it->first);

			// No data
			if(l.empty()){
				for(int j=0;j<num_fps;j++)
					fprintf(fps[j], "-1,");
				continue;
			}

			accesses[t_it->second] += l.size();

			// Calculates data
			int num_accesses = l.size();
			int min_latency = *(min_element(l.begin(), l.end()));
			double mean_latency = accumulate(l.begin(), l.end(), 0.0) / num_accesses;
			int max_latency = *(max_element(l.begin(), l.end()));

			// Prints data to files
			fprintf(fps[0], "%d,", num_accesses);
			fprintf(fps[1], "%d,", min_latency);
			fprintf(fps[2], "%.2f,", mean_latency);
			fprintf(fps[3], "%d,", max_latency);
		}

		for(int j=0;j<num_fps;j++)
			fprintf(fps[j], "\n");
	}

	// After all addresses (rows) are processed, we print number of accesses by each thread in the last row
	for(int i=0;i<num_fps;i++){
		fprintf(fps[i], "num_accesses,"); // Rowname
		for(int j=0;j<SYS_NUM_OF_CORES;j++)
			fprintf(fps[i], "%d,", accesses[j]);
	}

}

// Page vs how many different threads accessed to it?
void page_table_t::print_alt_graph(FILE *fp){
	// First line of the CSV file: column header
	fprintf(fp, "addr,threads_accessed\n");

	// Each unique page address will write a row with the data for each thread
	for (set<long int>::iterator it = uniq_addrs.begin(); it != uniq_addrs.end(); ++it){
		long int addr = *it;
		int threads_accessed = 0;		

		// A thread has accessed to the page if its cell is not null
		for(map<int, short>::iterator t_it = tid_index.begin(); t_it != tid_index.end(); ++t_it)
			threads_accessed += ( get_cell(addr, t_it->first) != NULL );

		// Prints row (row name and data) to file
		fprintf(fp, "%lx,%d\n", addr, threads_accessed);
	}
}

/*
---T1---
	node0		node1
t0	acc_0_n0	acc_0_n1
t1	acc_1_n0	acc_1_n1
...

acc_i_nj: thread_i accesses to pages in node_j
*/
void page_table_t::print_table1(){
	long int page_addr;
	table_cell_t cell;
	int counters[SYS_NUM_OF_MEMORIES];
	int page_node;

	// Initial print
	for(int i=0;i<SYS_NUM_OF_MEMORIES;i++)
		printf("\tN%d", i);
	printf("\n");

	// For each thread (row)...
	for(map<int, short>::iterator t_it = tid_index.begin(); t_it != tid_index.end(); ++t_it) {
		memset(counters, 0, sizeof(counters)); // Zero reset
		
		// For each map entry, gets address and sums accesses to counters[page_node]
		int pos = t_it->second;
		for(map<long int, table_cell_t>::iterator it = table[pos].begin(); it != table[pos].end(); ++it) {
			page_addr = it->first;
			cell = it->second;
			page_node = page_node_map[page_addr].current_place;
			counters[page_node] += cell.latencies.size(); // What if those accesses were done when page was in another node?...
		}

		// Prints results
		printf("T-%d:", t_it->first);
		for(int j=0;j<SYS_NUM_OF_MEMORIES;j++)
			printf("\t%d", counters[j]);
		printf("\n");
	}

	printf("\n");
}

/*
---T2---
	node0		node1
p0	acc_0_n0	acc_0_n1
p1	acc_1_n0	acc_1_n1
...

acc_i_nj: accesses to page_i from threads in node_j
*/
void page_table_t::print_table2(){
	int counters[SYS_NUM_OF_MEMORIES];

	// Initial print
	printf("\t");
	for(int i=0;i<SYS_NUM_OF_MEMORIES;i++)
		printf("\tN%d", i);
	printf("\n");

	// For each page...
	const int LIMIT = 50;
	int i = 0;

	for (set<long int>::iterator it = uniq_addrs.begin(); it != uniq_addrs.end(); ++it){
	//for (set<long int>::iterator it = uniq_addrs.end(); it != uniq_addrs.begin() && i<LIMIT; --it, i++){
		long int page_addr = *it;

		memset(counters, 0, sizeof(counters)); // Zero reset
		
		// For each thread, gets page cell and sums accesses to counters[cpu_node]
		for(map<int, short>::iterator t_it = tid_index.begin(); t_it != tid_index.end(); ++t_it) {
			table_cell_t* cell = get_cell(page_addr, t_it->first);
			if(cell != NULL){
				// For getting the memory cell, we need the core first
				int core = get_tid_core(t_it->first);
				int cpu_node = get_cpu_memory_cell(core);
				counters[cpu_node] += cell->latencies.size();
			}
		}

		// Prints results
		printf("P%lx:", page_addr);
		for(int j=0;j<SYS_NUM_OF_MEMORIES;j++)
			printf("\t%d", counters[j]);
		printf("\n");
	}

	printf("\n");
}

// Auxiliar function. It may slow the app
int get_median_from_list(vector<int> l){
	size_t size = l.size();
	sort(l.begin(), l.end()); // Getting median requires sorting

	if (size % 2 == 0)
		return (l[size / 2 - 1] + l[size / 2]) / 2;
	else 
		return l[size / 2];
}

/** The goal of these functions is to update page_node_map and tid_node_map so:
	  - We get from each memory page:
	    -- How many different threads accessed to it.
	    -- How many different threads accessed to it with a latency bigger than a threshold.
	    -- The average latency from those accesses.
	  - We get from each thread:
	    -- How many different memory pages it had accessed to.
	    -- How many different memory pages it had accessed with a latency bigger than a threshold to.
	    -- The average latency from those accesses.
	This info will be crucial to design a migration strategy.
	This implies looping over the table.
*/
// Fills only page_node_map
void page_table_t::calculate_performance_page(int threshold){
	// Loops over memory pages
	for (set<long int>::iterator it = uniq_addrs.begin(); it != uniq_addrs.end(); ++it){
		vector<int> l_thres;
		long int addr = *it;
		int threads_accessed = 0;
		int num_acs_thres = 0;

		// A thread has accessed to the page if its cell is not null
		for(map<int, short>::iterator t_it = tid_index.begin(); t_it != tid_index.end(); ++t_it) {
			vector<int> l = get_latencies_from_cell(addr, t_it->first);
			
			// No data
			if(l.empty())
				continue;

			threads_accessed++;

			// Calculates if there is at least a latency bigger than the threshold and keeps them
			bool upper_thres = 0;
			for(size_t j=0;j<l.size();j++){
				if(l[j] > threshold) {
					l_thres.push_back(l[j]);
					upper_thres = true;
				}
			}
			num_acs_thres += upper_thres;
			
		}

		// Updates data in map
		perf_data_t *cell = &page_node_map[addr];
		cell->num_uniq_accesses = threads_accessed;
		cell->num_acs_thres = num_acs_thres;

		if(!l_thres.empty()){
			cell->median_latency = get_median_from_list(l_thres);
			cell->min_latency = *(min_element(l_thres.begin(), l_thres.end()));
			cell->max_latency = *(max_element(l_thres.begin(), l_thres.end()));
		}
	}
}

// Fills only tid_node_map
void page_table_t::calculate_performance_tid(int threshold){
	// Loops over TIDs
	for(map<int, short>::iterator t_it = tid_index.begin(); t_it != tid_index.end(); ++t_it) {
		vector<int> l_thres;
		int pages_accessed = 0;
		int num_acs_thres = 0;

		// Loops over page entries for that TID
		pid_t tid = t_it->first;
		int pos = t_it->second;
		for (map<long int, table_cell_t>::iterator it = table[pos].begin(); it != table[pos].end(); ++it){
			vector<int> l = it->second.latencies;

			pages_accessed++;

			// Calculates if there is at least a latency bigger than the threshold and keeps them
			bool upper_thres = 0;
			for(size_t j=0;j<l.size();j++){
				if(l[j] > threshold) {
					l_thres.push_back(l[j]);
					upper_thres = true;
				}
			}
			num_acs_thres += upper_thres;
			
		}

		// After all the internal iterations ends, updates data in TID map
		perf_data_t *cell = &tid_node_map[tid];
		cell->current_place = get_tid_core(tid);
		cell->num_uniq_accesses = pages_accessed;
		cell->num_acs_thres = num_acs_thres;

		if(!l_thres.empty()){
			cell->median_latency = get_median_from_list(l_thres);
			cell->min_latency = *(min_element(l_thres.begin(), l_thres.end()));
			cell->max_latency = *(max_element(l_thres.begin(), l_thres.end()));
		}
	}
}

void page_table_t::print_performance(){
	for (auto it = page_node_map.begin(); it != page_node_map.end(); ++it){
		printf("%lx: ", it->first);
		it->second.print();
	}
	
	printf("\n");

	for (auto it = tid_node_map.begin(); it != tid_node_map.end(); ++it){
		printf("T%d: ", it->first);
		it->second.print();
	}
}

void perf_data::print(){
	printf("MEM_NODE/CORE: %d, UNIQ_ACS: %d, ACS_THRES: %d", current_place, num_uniq_accesses, num_acs_thres);

	if(num_acs_thres > 0)
		printf(", MIN_LAT: %d, MEDIAN_LAT: %d, MAX_LAT: %d", min_latency, median_latency, max_latency);
	printf("\n");
}