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
	printf("NUM_ACC %lu, MEAN_LAT %.2f, CACH_MIS: %u\n", latencies.size(), accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size(), cache_misses);
}

/*** page_table_t ***/
page_table_t::page_table(pid_t p){
	pid = p;
	table.resize(SYS_NUM_OF_CORES);
}

// Destructor. We clear everything
page_table_t::~page_table(){
		// Table itself
		for(column& c : table)
			c.clear();
		table.clear();

		// Vectors from maps, and maps itself
		for(auto& it : page_node_map)
			it.second.acs_per_node.clear();
		page_node_map.clear();

		for(auto& it : tid_node_map)
			it.second.acs_per_node.clear();
		tid_node_map.clear();

		table.clear();
		uniq_addrs.clear();
		tid_index.clear();
}

int page_table_t::add_cell(long int page_addr, int current_node, pid_t tid, int latency, int cpu, int cpu_node, bool is_cache_miss){
	table_cell_t *cell = get_cell(page_addr, tid);

	if(cell == NULL) {
		table_cell_t aux(latency, is_cache_miss);
		uniq_addrs.insert(page_addr); // Adds address to the set, won't be inserted if already added by other threads

		// If TID does not exist in map, we associate a index to it
		if(tid_index.count(tid) == 0){
			tid_index[tid] = tid_index.size();

			// Resizes vector (row) if necessary
			size_t ts = table.size();
			if(ts == tid_index.size())
				table.resize(2*ts);
		}
		int pos = tid_index[tid];
		table[pos][page_addr] = aux;
	} else
		cell->update(latency, is_cache_miss);
	
	// Creates/updates association in page node map
	perf_data_t *pd = &page_node_map[page_addr];
	pd->current_place = current_node;
	pd->acs_per_node[cpu_node]++;

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

// Somewhat ugly. You can't just remove a single TID without iterating because you need to change the following indexes
void page_table_t::remove_tid(pid_t tid){
	bool erased = false;

	for(map<int, short>::iterator it = tid_index.begin(); it != tid_index.end(); ++it) {
		pid_t it_tid = it->first;
		int pos = it->second;

		// Removing a thread (row) implies deleting the map association (also in system struct), deleting the element in the vector, and updating following positions
		if(tid == it_tid){
			printf("TID %d will be removed from the table.\n", tid);
			tid_index.erase(it);
			++it;
			table.erase(table.begin() + pos - erased);
			erased = true;
			remove_tid(tid); // From system struct
		}
		else
			tid_index[it_tid] = pos - erased;
	}
}

void page_table_t::remove_inactive_tids(){
	unsigned int erased = 0;

	for(map<int, short>::iterator it = tid_index.begin(); it != tid_index.end(); ++it) {
		pid_t tid = it->first;
		int pos = it->second;

		// Removing a thread (row) implies deleting the map association (also in system struct), deleting the element in the vector, and updating following positions
		if(!is_tid_alive(pid, tid)){
			printf("TID %d is dead. It will be removed from the table.\n", tid);
			tid_index.erase(it);
			++it;
			table.erase(table.begin() + pos - erased);
			erased++;
			remove_tid(tid); // From system struct
		}
		else
			tid_index[tid] = pos - erased;
	}
}

void page_table_t::print(){
	printf("Page table for PID: %d\n", pid);

	// Prints each row (TID)
	for(auto const & t_it : tid_index) {
		int tid = t_it.first;
		int pos = t_it.second;

		printf("TID: %d\n", tid);
		if(table[pos].empty())
			printf("\tEmpty.\n");
		
		// Prints each cell (TID + page address)
		for(auto const & it : table[pos]) {
			long int page_addr = it.first;
			table_cell_t cell = it.second;
			
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
		for(auto const & t_it : tid_index)
			fprintf(fps[i], "T_%d,", t_it.first);
		fprintf(fps[i], "\n");
	}

	// Each unique page address will write a row with the data for each thread
	for (long int const & addr : uniq_addrs){

		// Writes address as row name (first column)
		for(int i=0;i<num_fps;i++)
			fprintf(fps[i], "%lx,", addr);

		// Writes data for each thread
		for(auto const & t_it : tid_index) {
			vector<int> l = get_latencies_from_cell(addr, t_it.first);

			// No data
			if(l.empty()){
				for(int j=0;j<num_fps;j++)
					fprintf(fps[j], "-1,");
				continue;
			}

			accesses[t_it.second] += l.size();

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
	for (long int const & addr : uniq_addrs){
		int threads_accessed = 0;		

		// A thread has accessed to the page if its cell is not null
		for(auto const & t_it : tid_index)
			threads_accessed += ( get_cell(addr, t_it.first) != NULL );

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
	for(auto t_it : tid_index) {
		memset(counters, 0, sizeof(counters)); // Zero reset
		
		// For each map entry, gets address and sums accesses to counters[page_node]
		int pos = t_it.second;
		for(auto it : table[pos]) {
			page_addr = it.first;
			cell = it.second;
			page_node = page_node_map[page_addr].current_place;
			counters[page_node] += cell.latencies.size(); // What if those accesses were done when page was in another node?...
		}

		// Prints results
		printf("T-%d:", t_it.first);
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
		for(auto const & t_it : tid_index) {
			table_cell_t* cell = get_cell(page_addr, t_it.first);
			if(cell != NULL){
				// For getting the memory cell, we need the core first
				int core = get_tid_core(t_it.first);
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
		for(auto const & t_it : tid_index) {
			vector<int> l = get_latencies_from_cell(addr, t_it.first);
			
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
		for (auto const & it : table[pos]){
			vector<int> l = it.second.latencies;

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
	for (auto const & it : page_node_map){
		printf("%lx: ", it.first);
		it.second.print();
	}
	
	printf("\n");

	for (auto const & it : tid_node_map){
		printf("T-%d: ", (int) it.first);
		it.second.print();
	}
}

/*** perf_data_t functions ***/
void perf_data_t::print() const {
	printf("MEM_NODE/CORE: %d, ACS_PER_NODE: {", current_place);

	for(size_t i=0;i<SYS_NUM_OF_MEMORIES;i++)
		printf(" %d", acs_per_node[i]);
	printf(" }");

	if(num_acs_thres > 0)
		printf(", UNIQ_ACS: %d, ACS_THRES: %d, MIN_LAT: %d, MEDIAN_LAT: %d, MAX_LAT: %d", num_uniq_accesses, num_acs_thres, min_latency, median_latency, max_latency);
	printf("\n");
}
