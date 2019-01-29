#pragma once
#ifndef __PAGE_TABLE__
#define __PAGE_TABLE__

#include <iostream>

// Data structures
#include <vector>
#include <map>
#include <set>

#include <numeric> // accumulate
#include <algorithm> // min_element and max_element

#include "system_struct.hpp" // Needs to know the system
#include "migration_cell.hpp"
#include "performance.hpp"
#include "utils.hpp"

class table_cell_t {
public:
	std::vector<int> latencies;
	unsigned cache_misses;

	table_cell_t () {};

	table_cell_t (const int latency, const bool is_cache_miss) {
		latencies.push_back(latency);
		cache_misses = (unsigned) is_cache_miss;
	}

	inline void update (const int latency, const bool is_cache_miss) {
		latencies.push_back(latency);
		cache_misses += (unsigned) is_cache_miss;
	}

	void print () const {
		std::cout << (*this);
	}

	friend std::ostream & operator << (std::ostream & os, const table_cell_t & tc) {
		const auto precision = os.precision();
		os.precision(3);
		os << "NUM_ACC " << tc.latencies.size() << ", MEAN_LAT " << std::accumulate(tc.latencies.begin(), tc.latencies.end(), 0.0) << ", CACH_MIS: " << tc.cache_misses << '\n';
		os.precision(precision);
		return os;
	}
};

typedef std::map<long int, table_cell_t> column; // Each column of the table, defined with typedef for readibility

class page_table_t {
public:
	std::vector<column> table; // Matrix/table where rows are threads (uses index from tid_index) and columns are indexed by page address
	std::set<long int> uniq_addrs; // All different addresses used in this table, useful for heatmap printing
	std::map<pid_t, short> tid_index; // Translates TID to row index

	std::map<long int, pg_perf_data_t> page_node_map; // Maps page address to memory node and other data (may be redundant with page_node_table)

	std::map<pid_t, rm3d_data_t> perf_per_tid; // Maps TID to Óscar's performance data

	perf_table_t page_node_table; // Experimental table for memory pages

	pid_t pid;

	page_table_t () {};

	page_table_t (const pid_t p) :
		page_node_table(system_struct_t::NUM_OF_MEMORIES),
		pid(p)
	{
		table.resize(system_struct_t::NUM_OF_CPUS);
	};

	~page_table_t () {
		// Table itself
		// for (column & c : table)
		// 	c.clear();
		//
		// table.clear();
		//
		// // Vectors from maps, and maps itself
		// for (auto & it : page_node_map)
		// 	it.second.acs_per_node.clear();
		//
		// page_node_map.clear();
		//
		// for (auto & it : perf_per_tid) {
		// 	rm3d_data_t & pd = it.second;
		// 	pd.insts.clear();
		// 	pd.reqs.clear();
		// 	pd.times.clear();
		// 	pd.v_perfs.clear();
		// }
		// perf_per_tid.clear();
		//
		// table.clear();
		// uniq_addrs.clear();
		// tid_index.clear();
	}

	int add_cell (const long int page_addr, const int current_node,  const pid_t tid, const int latency, const int cpu, const int cpu_node, const bool is_cache_miss) {
		table_cell_t * cell = get_cell(page_addr, tid);

		if (cell == nullptr) {
			table_cell_t aux(latency, is_cache_miss);
			uniq_addrs.insert(page_addr); // Adds address to the set, won't be inserted if already added by other threads

			// If TID does not exist in map, we associate a index to it
			if (tid_index.count(tid) == 0) {
				tid_index[tid] = tid_index.size();

				// Resizes vector (row) if necessary
				size_t ts = table.size();
				if (ts == tid_index.size())
					table.resize(2*ts);
			}
			int pos = tid_index[tid];
			table[pos][page_addr] = aux;
		} else {
			cell->update(latency, is_cache_miss);
		}

		// Creates/updates association in page node map
		pg_perf_data_t & pd = page_node_map[page_addr];
		pd.last_cpu_access = cpu;
		pd.current_node = current_node;
		pd.acs_per_node[cpu_node]++;

		// Creates/updates association in experimental table
		// [TOTHINK]: right now, we are storing accesses and latencies using the node where the page is, maybe it should be done regarding the CPU source of the access?
		page_node_table.add_data(page_addr, current_node, latency);

		return 0;
	}

	inline bool contains_addr (const long int page_addr, const int tid) {
		if(tid_index.count(tid) == 0)
			return false;

		int pos = tid_index[tid];
		return table[pos].count(page_addr) > 0;
	}

	table_cell_t * get_cell(const long int page_addr, const int tid) {
		if (contains_addr(page_addr, tid)) {
			int pos = tid_index[tid];
			return &table[pos][page_addr];
		} else
			return nullptr;
	}

	std::vector<int> get_latencies_from_cell (const long int page_addr, const int tid) {
		if (contains_addr(page_addr,tid)) {
			int pos = tid_index[tid];
			return table[pos][page_addr].latencies;
		} else {
			std::vector<int> v;
			return v;
		}
	}

	void remove_tid (const pid_t tid) {
		bool erased = false;

		for (std::map<int, short>::iterator it = tid_index.begin(); it != tid_index.end(); ++it) {
			pid_t it_tid = it->first;
			int pos = it->second;

			// Removing a thread (row) implies deleting the map association (also in system struct), deleting the element in the vector, and updating following positions
			if (tid == it_tid) {
				std::cout << "TID " << tid << " will be removed from the table." << '\n';
				tid_index.erase(it);
				++it;
				table.erase(table.begin() + pos - erased);
				erased = true;
				system_struct_t::remove_tid(tid, false);
			}
			else
				tid_index[it_tid] = pos - erased;
		}
	}

	void remove_finished_tids (const bool unpin_3drminactive_tids) {
		unsigned int erased = 0;

		for(auto it = tid_index.begin(); it != tid_index.end(); ) {
			pid_t tid = it->first;
			int pos = it->second;

			// Removing a thread (row) implies deleting the map association (also in system struct), deleting the element in the vector, and updating following positions
			if (!is_tid_alive(pid, tid)) {
				std::cout << "TID " << tid << " is dead. It will be removed from the table." << '\n';
				it = tid_index.erase(it); // For erasing correctly

				table.erase(table.begin() + pos - erased);
				erased++;
				system_struct_t::remove_tid(tid, false);
			}
			else {
				++it; // For erasing correctly

				// Only when we use annealing strategy
				if(unpin_3drminactive_tids && !perf_per_tid[tid].active)
					system_struct_t::remove_tid(tid, true);

				tid_index[tid] = pos - erased;
			}
		}
	}

	inline std::vector<pid_t> get_tids () const {
		std::vector<pid_t> v;

		for (auto const & t_it : tid_index)
			v.push_back(t_it.first);

		return v;
	}

	void print () const {
		std::cout << *this;
	}

	void calculate_performance_page (const int threshold) {
		// Loops over memory pages
		for (long int const & addr : uniq_addrs) {
			std::vector<int> l_thres;
			int threads_accessed = 0;
			int num_acs_thres = 0;

			// A thread has accessed to the page if its cell is not null
			for (auto const & t_it : tid_index) {
				std::vector<int> l = get_latencies_from_cell(addr, t_it.first);

				// No data
				if (l.empty())
					continue;

				threads_accessed++;

				// Calculates if there is at least a latency bigger than the threshold and keeps them
				bool upper_thres = 0;
				for (size_t j = 0; j < l.size(); j++) {
					if (l[j] > threshold) {
						l_thres.push_back(l[j]);
						upper_thres = true;
					}
				}
				num_acs_thres += upper_thres;
			}

			// Updates data in map
			pg_perf_data_t & cell = page_node_map[addr];
			cell.num_uniq_accesses = threads_accessed;
			cell.num_acs_thres = num_acs_thres;

			if (!l_thres.empty()) {
				cell.median_latency = get_median_from_list(l_thres);
				cell.min_latency = *(std::min_element(l_thres.begin(), l_thres.end()));
				cell.max_latency = *(std::max_element(l_thres.begin(), l_thres.end()));
			}
		}
	}

	inline void print_performance () const {
		std::cout << "Page table por PID: " << pid << '\n';
		std::cout << page_node_table;
	}

	void update_page_locations (const std::vector<migration_cell_t> & pg_migr) {
		for (migration_cell_t const & pgm : pg_migr){
			long int addr = pgm.elem;
			short new_dest = pgm.dest;
			page_node_map[addr].current_node = new_dest;
		}
	}

	inline void update_page_location (const migration_cell_t pgm) {
		long int addr = pgm.elem;
		short new_dest = pgm.dest;
		page_node_map[addr].current_node = new_dest;
	}

	// Code made for replicating Óscar's work
	inline void add_inst_data_for_tid (const pid_t tid, const int core, const long int insts, const long int req_dr, const long int time) {
		perf_per_tid[tid].add_data(core, insts, req_dr, time);
	}

	void calc_perf () { // Óscar's definition of performance
		// We loop over TIDs
		for (auto const & t_it : tid_index) {
			pid_t tid = t_it.first;

			if (!perf_per_tid[tid].active) // Only for active
				continue;

			std::vector<int> v = get_lats_for_tid(tid);
			double mean_lat = accumulate(v.begin(), v.end(), 0.0) / v.size();

			// Performance is updated
			perf_per_tid[tid].calc_perf(mean_lat);
		}
	}

	pid_t normalize_perf_and_get_worst_thread () {
		double min_p = 1e15;
		pid_t min_t = -1;

		int active_threads = 0;
		double mean_perf = 0.0;

		// We loop over TIDs
		for (auto const & t_it : tid_index) {
			pid_t tid = t_it.first;
			rm3d_data_t pd = perf_per_tid[tid];

			if(!pd.active)
				continue;

			active_threads++;

			// We sum the last_perf if active
			double pd_lp = pd.v_perfs[pd.index_last_node_calc];
			mean_perf += pd_lp;
			if(pd_lp < min_p){ // And we calculate the minimum
				min_t = tid;
				min_p = pd_lp;
			}
		}

		if(active_threads == 0) // Should never happen
			return -1;

		mean_perf /= active_threads;

		// We loop over TIDs again for normalising
		for (auto const & t_it : tid_index) {
			pid_t tid = t_it.first;
			rm3d_data_t & pd = perf_per_tid[tid];

			if(!pd.active)
				continue;

			pd.v_perfs[pd.index_last_node_calc] /= mean_perf;
		}

		return min_t;
	}

	void reset_performance () {
		// We loop over TIDs
		for (auto const & t_it : tid_index) {
			pid_t tid = t_it.first;

			// And we call reset for them
			perf_per_tid[tid].reset();
		}
	}

	double get_total_performance () {
		double val = 0.0;

		// We loop over TIDs
		for (auto const & t_it : tid_index) {
			pid_t tid = t_it.first;

			// And we sum them all if active
			rm3d_data_t pd = perf_per_tid[tid];
			if(!pd.active)
				continue;

			val += pd.v_perfs[pd.index_last_node_calc];
		}

		return val;
	}

	inline std::vector<double> get_perf_data(const pid_t tid) {
		return perf_per_tid[tid].v_perfs;
	}

	std::vector<int> get_all_lats () {
		std::vector<int> v;
		v.reserve(2 * uniq_addrs.size());

		// We loop over TIDs
		for (auto const & t_it : tid_index) {
			const std::vector<int> & ls = get_lats_for_tid(t_it.first);
			v.insert(v.end(), ls.begin(), ls.end()); // Appends latencies to main list
		}

		return v;
	}

	std::vector<int> get_lats_for_tid (const pid_t tid) {
		std::vector<int> v;

		int pos = tid_index[tid];
		for (auto const & it : table[pos]){
			const std::vector<int> ls = it.second.latencies;
			v.insert(v.end(), ls.begin(), ls.end()); // Appends latencies to main list
		}

		return v;
	}

	double get_mean_acs_to_pages () {
		std::vector<short> v;

		// We collect the sums of the accesses per node for each page
		for (auto const & it : page_node_map){
			const std::vector<unsigned short> & vaux = it.second.acs_per_node;
			int num_acs = accumulate(vaux.begin(), vaux.end(), 0);
			v.push_back(num_acs);
		}

		// ... and then we calculate the mean
		return accumulate(v.begin(), v.end(), 0.0) / v.size();
	}

	inline double get_mean_lat_to_pages () {
		std::vector<int> v = get_all_lats();
		return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
	}

	// More info about the definitions in source file
	void print_heatmaps (FILE **fps, int num_fps) {
		// TODO
	}
	void print_alt_graph (FILE *fp) {
		// TODO
	}

	friend std::ostream & operator << (std::ostream & os, const page_table_t & pg) {
		os << "Page table for PID: " << pg.pid << '\n';

		// Prints each row (TID)
		for (auto const & t_it : pg.tid_index) {
			pid_t tid = t_it.first;
			int pos = t_it.second;

			os << "TID: " << tid << '\n';
			if (pg.table[pos].empty())
				os << "\tEmpty.\n";

			// Prints each cell (TID + page address)
			for (auto const & it : pg.table[pos]) {
				long int page_addr = it.first;
				const table_cell_t cell = it.second;
				const auto current_node = pg.page_node_map.at(page_addr).current_node;

				os << "\tPAGE_ADDR: " << page_addr << "x, CURRENT_NODE: " << current_node << ", ";
				os << cell;
			}
			os << '\n';
		}
		return os;
	}
};

#endif
