#include "page_table.h"

/*** table_cell_t ***/
table_cell_t::table_cell(pid_t tid, int latency, int cpu, bool is_cache_miss){
	latencies.push_back(latency);
	cache_misses = (unsigned) is_cache_miss;

	//accesses_l.add_cell(tid, latency, cpu);
}

int table_cell_t::update(pid_t tid, int latency, int cpu, bool is_cache_miss){
	latencies.push_back(latency);
	cache_misses += (unsigned) is_cache_miss;

	//accesses_l.add_cell(tid, latency, cpu);
	return 2;
}

void table_cell_t::print(){
	printf("M_LAT %.2f, T_ACC %lu, CACH_MIS: %u\n", accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size(), latencies.size(), cache_misses);
	//accesses_l.print();
}

/*** page_table_t ***/
int page_table_t::add_cell(long int page_addr, int current_node, pid_t tid, int latency, int cpu, int cpu_node, bool is_cache_miss){
	table_cell_t *cell = get_cell(page_addr, cpu);

	if(cell == NULL) {
		table_cell_t aux(tid,latency,cpu,is_cache_miss);
		table[cpu][page_addr] = aux;
		uniq_addrs.insert(page_addr); // Adds address to the set, won't be inserted if already added by other threads
	} else
		cell->update(tid,latency,cpu,is_cache_miss);
	
	page_node_map[page_addr].current_mem_node = current_node; // Creates/updates association in page node map

	return 0;
}

bool page_table_t::contains_addr(long int page_addr, int cpu){
	return table[cpu].count(page_addr) > 0;
}

vector<int> page_table_t::get_latencies_from_cell(long int page_addr, int cpu){
	if(contains_addr(page_addr,cpu))
		return table[cpu][page_addr].latencies;
	else {
		vector<int> v;
		return v;
	}
}


// Returns NULL if no association
table_cell_t* page_table_t::get_cell(long int page_addr, int cpu){
	if(contains_addr(page_addr,cpu))
		return &table[cpu][page_addr];
	else
		return NULL;
}

int page_table_t::reset_cell(long int page_addr, int current_node){
	table_cell_t *cell = NULL;
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		cell = get_cell(page_addr, i);
		if(cell != NULL)
			cell->latencies.clear();
	}
	
	page_node_map[page_addr].current_mem_node = current_node;
	return 0;
}

void page_table_t::clear(){
	for(int i=0;i<SYS_NUM_OF_CORES;i++)
		table[i].clear();
}

void page_table_t::print(){
	printf("page table\n");

	long int page_addr;
	table_cell_t cell;
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		if(table[i].empty())
			printf("empty for %d cpu", i);
		
		for(map<long int, table_cell_t>::iterator it = table[i].begin(); it != table[i].end(); ++it) {
			page_addr = it->first;
			cell = it->second;
			
			printf("PAGE_ADDR: %lx, CURRENT_NODE: %d, ", page_addr, page_node_map[page_addr].current_mem_node);
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
		for(int j=0;j<SYS_NUM_OF_CORES;j++)
			fprintf(fps[i], "T%d,", j);
		fprintf(fps[i], "\n");
	}

	// Each unique page address will write a row with the data for each thread
	for (set<long int>::iterator it = uniq_addrs.begin(); it != uniq_addrs.end(); ++it){
		long int addr = *it;

		// Writes address as row name (first column)
		for(int i=0;i<num_fps;i++)
			fprintf(fps[i], "%lx,", addr);

		// Writes data for each thread/core
		for(int i=0;i<SYS_NUM_OF_CORES;i++){
			vector<int> l = get_latencies_from_cell(addr,i);

			// No data
			if(l.empty()){
				for(int j=0;j<num_fps;j++)
					fprintf(fps[j], "-1,");
				continue;
			}

			accesses[i] += l.size();

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

		// A thread/CPU has accessed to the page if its cell is not null
		for(int i=0;i<SYS_NUM_OF_CORES;i++)
			threads_accessed += ( get_cell(addr,i) != NULL );

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

acc_i_nj: thread/cpu_i accesses to pages in node_j
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

	// For each CPU (row)...
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		memset(counters, 0, sizeof(counters)); // Zero reset
		
		// For each map entry, gets address and sums accesses to counters[page_node]
		for(map<long int, table_cell_t>::iterator it = table[i].begin(); it != table[i].end(); ++it) {
			page_addr = it->first;
			cell = it->second;
			page_node = page_node_map[page_addr].current_mem_node;
			counters[page_node] += cell.latencies.size(); // What if those accesses were done when page was in another node?...
		}

		// Prints results
		printf("T%d:", i);
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

acc_i_nj: accesses to page_i from threads/cpus in node_j
*/
void page_table_t::print_table2(){
	long int page_addr;
	table_cell_t* cell;
	int counters[SYS_NUM_OF_MEMORIES];
	int cpu_node;

	// Initial print
	printf("\t");
	for(int i=0;i<SYS_NUM_OF_MEMORIES;i++)
		printf("\tN%d", i);
	printf("\n");

	// For each page...
	const int LIMIT = 50;
	int i = 0;

	//for (set<long int>::iterator it = uniq_addrs.begin(); it != uniq_addrs.end(); ++it){
	for (set<long int>::iterator it = uniq_addrs.end(); it != uniq_addrs.begin() && i<LIMIT; --it, i++){
		page_addr = *it;

		memset(counters, 0, sizeof(counters)); // Zero reset
		
		// For each CPU, gets page cell and sums accesses to counters[cpu_node]
		for(int j=0;j<SYS_NUM_OF_CORES;j++){

			// TODO: for now works well for CPUs, then this will be changed to TIDs
			cpu_node = get_cpu_memory_cell(j);
			cell = get_cell(page_addr, j);
			if(cell != NULL)
				counters[cpu_node] += cell->latencies.size();
		}

		// Prints results
		printf("P%lx:", page_addr);
		for(int j=0;i<SYS_NUM_OF_MEMORIES;j++)
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

// Fills only page_node_map
void page_table_t::calculate_performance_page(int threshold){
	// Loops over memory pages
	for (set<long int>::iterator it = uniq_addrs.begin(); it != uniq_addrs.end(); ++it){
		vector<int> l_thres;
		long int addr = *it;
		int threads_accessed = 0;
		int num_acs_thres = 0;

		// A thread/CPU has accessed to the page if its cell is not null
		for(int i=0;i<SYS_NUM_OF_CORES;i++){
			vector<int> l = get_latencies_from_cell(addr,i);
			
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
	// Loops over cores/TIDs
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		vector<int> l_thres;
		int pages_accessed = 0;
		int num_acs_thres = 0;

		// Loops over page entries for that core/TID
		for (map<long int, table_cell_t>::iterator it = table[i].begin(); it != table[i].end(); ++it){
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
		perf_data_t *cell = &tid_node_map[i]; // For now works, but it should be used TIDs instead of CPU IDs
		cell->current_mem_node = get_cpu_memory_cell(i); // Bad! It should get TID's node, not CPU's. This issue will require a big rewrite
		cell->num_uniq_accesses = pages_accessed;
		cell->num_acs_thres = num_acs_thres;

		if(!l_thres.empty()){
			cell->median_latency = get_median_from_list(l_thres);
			cell->min_latency = *(min_element(l_thres.begin(), l_thres.end()));
			cell->max_latency = *(max_element(l_thres.begin(), l_thres.end()));
		}
	}
}


/** [TODO]
	The goal of this function is to update page_node_map and tid_node_map so:
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
void page_table_t::calculate_performance(int threshold){

	map<int, int> threads_accessed;
	
	// Loops over cores/TIDs
	for(int i=0;i<SYS_NUM_OF_CORES;i++){
		vector<int> l_thres;
		int pages_accessed = 0;
		int num_acs_thres = 0;

		// Loops over page entries for that core/TID
		for (map<long int, table_cell_t>::iterator it = table[i].begin(); it != table[i].end(); ++it){
			long int addr = it->first;
			vector<int> l = it->second.latencies;

			// An entry in the table means memory page "addr" has ben accessed by one more thread
			threads_accessed[addr]++;
			
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
		perf_data_t *cell = &tid_node_map[i];
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
	printf("MEM_NODE %d, UNIQ_ACS %d, ACS_THRES: %d", current_mem_node, num_uniq_accesses, num_acs_thres);

	if(num_acs_thres > 0)
		printf(", MIN_LAT: %d, MEDIAN_LAT: %d, MAX_LAT: %d", min_latency, median_latency, max_latency);
	printf("\n");
}
