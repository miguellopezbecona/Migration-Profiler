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

#include "types_definition.hpp"
#include "system_struct.hpp" // Needs to know the system
#include "migration_cell.hpp"
#include "performance.hpp"
#include "utils.hpp"

class table_cell_t {
public:
	std::vector<lat_t> latencies;
	size_t cache_misses;

	table_cell_t () :
	 	latencies()
	{};

	table_cell_t (const lat_t latency, const bool is_cache_miss) :
	 	latencies(1, latency),
		cache_misses(unsigned(is_cache_miss))
	{}

	inline void update (const lat_t latency, const bool is_cache_miss) {
		latencies.push_back(latency);
		cache_misses += (unsigned) is_cache_miss;
	}

	inline void print () const {
		std::cout << *this << '\n';
	}

	friend std::ostream & operator << (std::ostream & os, const table_cell_t & tc) {
		os.precision(2); os << std::fixed;
		os << "NUM_ACC " << tc.latencies.size() << ", MEAN_LAT " << std::accumulate(tc.latencies.begin(), tc.latencies.end(), 0.0) << ", CACH_MIS: " << tc.cache_misses;
		os << std::defaultfloat;
		return os;
	}
};

using column = std::map<addr_t, table_cell_t>; // Each column of the table, defined with using for readibility

class page_table_t {
public:
	std::vector<column> table; // Matrix/table where rows are threads (uses index from tid_index) and columns are indexed by page address
	std::set<addr_t> uniq_addrs; // All different addresses used in this table, useful for heatmap printing
	std::map<tid_t, size_t> tid_index; // Translates TID to row index

	std::map<addr_t, pg_perf_data_t> page_node_map; // Maps page address to memory node and other data (may be redundant with page_node_table)

	std::map<pid_t, rm3d_data_t> perf_per_tid; // Maps TID to Óscar's performance data

	perf_table_t page_node_table; // Experimental table for memory pages

	pid_t pid;

	page_table_t () :
		table(system_struct_t::NUM_OF_CPUS),
		uniq_addrs(),
		tid_index(),
		page_node_map(),
		perf_per_tid(),
		page_node_table(system_struct_t::NUM_OF_MEMORIES),
		pid()
	{
		// table.reserve(system_struct_t::NUM_OF_CPUS);
	};

	page_table_t (const pid_t p) :
		table(system_struct_t::NUM_OF_CPUS),
		uniq_addrs(),
		tid_index(),
		page_node_map(),
		perf_per_tid(),
		page_node_table(system_struct_t::NUM_OF_MEMORIES),
		pid(p)
	{};

	~page_table_t () {}

	int add_cell (const addr_t page_addr, const node_t current_node, const pid_t tid, const lat_t latency, const cpu_t cpu, const node_t cpu_node, const bool is_cache_miss) {
		auto * cell = get_cell(page_addr, tid);

		if (cell == nullptr) {
			uniq_addrs.insert(page_addr); // Adds address to the set, won't be inserted if already added by other threads

			// If TID does not exist in map, we associate a index to it
			if (tid_index.count(tid) == 0) {
				tid_index[tid] = tid_index.size();

				// Resizes vector (row) if necessary
				const auto ts = table.size();
				if (ts == tid_index.size())
					table.resize(2*ts);
			}
			const auto pos = tid_index[tid];
			table[pos][page_addr] = table_cell_t(latency, is_cache_miss);
		} else {
			cell->update(latency, is_cache_miss);
		}

		// Creates/updates association in page node map
		auto & pd = page_node_map[page_addr];
		pd.last_cpu_access = cpu;
		pd.current_node = current_node;
		pd.acs_per_node[cpu_node]++;

		// Creates/updates association in experimental table
		// [TOTHINK]: right now, we are storing accesses and latencies using the node where the page is, maybe it should be done regarding the CPU source of the access?
		page_node_table.add_data(page_addr, current_node, latency);

		return 0;
	}

	inline bool contains_addr (const addr_t page_addr, const tid_t tid) {
		if (tid_index.count(tid) == 0)
			return false;

		const auto pos = tid_index[tid];
		return table[pos].count(page_addr) > 0;
	}

	inline table_cell_t * get_cell (const addr_t page_addr, const tid_t tid) {
		if (contains_addr(page_addr, tid)) {
			const auto pos = tid_index[tid];
			return &(table[pos][page_addr]);
		} else
			return nullptr;
	}

	inline std::vector<lat_t> get_latencies_from_cell (const addr_t page_addr, const pid_t tid) {
		if (contains_addr(page_addr,tid)) {
			const auto pos = tid_index[tid];
			return table[pos][page_addr].latencies;
		} else {
			return std::vector<lat_t>();
		}
	}

	void remove_tid (const pid_t tid) {
		bool erased = false;

		for (auto it = tid_index.begin(); it != tid_index.end(); ++it) {
			const auto it_tid = it->first;
			const auto pos = it->second;

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
		size_t erased = 0;

		for (auto it = tid_index.begin(); it != tid_index.end(); ) {
			const auto tid = it->first;
			const auto pos = it->second;

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
				if (unpin_3drminactive_tids && !perf_per_tid[tid].active)
					system_struct_t::remove_tid(tid, true);

				tid_index[tid] = pos - erased;
			}
		}
	}

	inline std::vector<pid_t> get_tids () const {
		std::vector<pid_t> v;

		for (const auto & t_it : tid_index)
			v.push_back(t_it.first);

		return v;
	}

	inline void print () const {
		std::cout << *this << '\n';
	}

	template<class T>
	void calculate_performance_page (const T threshold) {
		// Loops over memory pages
		for (const auto & addr : uniq_addrs) {
			std::vector<lat_t> l_thres;
			size_t threads_accessed = 0;
			size_t num_acs_thres = 0;

			// A thread has accessed to the page if its cell is not null
			for (const auto & t_it : tid_index) {
				const auto l = get_latencies_from_cell(addr, t_it.first);

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
			auto & cell = page_node_map[addr];
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
			const auto addr = pgm.elem;
			const auto new_dest = pgm.dest;
			page_node_map[addr].current_node = new_dest;
		}
	}

	inline void update_page_location (const migration_cell_t & pgm) {
		const auto addr = pgm.elem;
		const auto new_dest = pgm.dest;
		page_node_map[addr].current_node = new_dest;
	}

	// Code made for replicating Óscar's work
	inline void add_inst_data_for_tid (const tid_t tid, const cpu_t core, const ins_t insts, const size_t req_dr, const tim_t time) {
		perf_per_tid[tid].add_data(core, insts, req_dr, time);
	}

	void calc_perf () { // Óscar's definition of performance
		// We loop over TIDs
		for (const auto & t_it : tid_index) {
			pid_t tid = t_it.first;

			if (!perf_per_tid[tid].active) // Only for active
				continue;

			const auto v = get_lats_for_tid(tid);
			double mean_lat = std::accumulate(v.begin(), v.end(), 0.0) / v.size();

			// Performance is updated
			perf_per_tid[tid].calc_perf(mean_lat);
		}
	}

	pid_t normalize_perf_and_get_worst_thread () {
		double min_p = 1e15;
		pid_t min_t = -1;

		size_t active_threads = 0;
		double mean_perf = 0.0;

		// We loop over TIDs
		for (const auto & t_it : tid_index) {
			const auto tid = t_it.first;
			const auto & pd = perf_per_tid[tid];

			if (!pd.active)
				continue;

			active_threads++;

			// We sum the last_perf if active
			const auto pd_lp = pd.v_perfs[pd.index_last_node_calc];
			mean_perf += pd_lp;
			if (pd_lp < min_p) { // And we calculate the minimum
				min_t = tid;
				min_p = pd_lp;
			}
		}

		mean_perf /= active_threads;

		// We loop over TIDs again for normalising
		for (const auto & t_it : tid_index) {
			const auto tid = t_it.first;
			auto & pd = perf_per_tid[tid];

			if (!pd.active)
				continue;

			pd.v_perfs[pd.index_last_node_calc] /= mean_perf;
		}

		return min_t;
	}

	void reset_performance () {
		// We loop over TIDs
		for (const auto & t_it : tid_index) {
			const auto tid = t_it.first;

			// And we call reset for them
			perf_per_tid[tid].reset();
		}
	}

	double get_total_performance () const {
		double val = 0.0;

		// We loop over TIDs
		for (const auto & t_it : tid_index) {
			const auto tid = t_it.first;

			// And we sum them all if active
			const auto & pd = perf_per_tid.at(tid);
			if (!pd.active)
				continue;

			val += pd.v_perfs.at(pd.index_last_node_calc);
		}

		return val;
	}

	inline std::vector<double> get_perf_data (const tid_t tid) const {
		// May happen that there is no value with key "tid". ¿ERROR?
		return (perf_per_tid.find(tid) != perf_per_tid.end()) ?
				perf_per_tid.at(tid).v_perfs : std::vector<double>();
	}

	std::vector<lat_t> get_all_lats () const {
		std::vector<lat_t> v;
		v.reserve(2 * uniq_addrs.size());

		// We loop over TIDs
		for (const auto & t_it : tid_index) {
			const auto ls = get_lats_for_tid(t_it.first);
			v.insert(v.end(), ls.begin(), ls.end()); // Appends latencies to main list
		}

		return v;
	}

	std::vector<lat_t> get_lats_for_tid (const tid_t tid) const {
		std::vector<lat_t> v;

		const auto pos = tid_index.at(tid);
		for (const auto & it : table[pos]) {
			const auto & ls = it.second.latencies;
			v.insert(v.end(), ls.begin(), ls.end()); // Appends latencies to main list
		}

		return v;
	}

	double get_mean_acs_to_pages () {
		std::vector<size_t> v;

		// We collect the sums of the accesses per node for each page
		for (const auto & it : page_node_map) {
			const auto & vaux = it.second.acs_per_node;
			const auto num_acs = std::accumulate(vaux.begin(), vaux.end(), 0);
			v.push_back(num_acs);
		}

		// ... and then we calculate the mean
		return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
	}

	inline double get_mean_lat_to_pages () {
		const auto & v = get_all_lats();
		return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
	}

	// More info about the definitions in source file
	void print_heatmaps (FILE ** fps, size_t num_fps) {
		// This is for writing number of accesses by each thread in the last row
		size_t * accesses = new size_t[tid_index.size()]{};

		// First line of each CSV file: column header
		for (size_t i = 0; i < num_fps; i++){
			// Rowname (first column)
			fprintf(fps[i], "addr,");

			// Threads' ID
			for (const auto & t_it : tid_index)
				fprintf(fps[i], "T_%d,", t_it.first);
			fprintf(fps[i], "\n");
		}

		// Each unique page address will write a row with the data for each thread
		for (long int const & addr : uniq_addrs) {

			// Writes address as row name (first column)
			for (size_t i = 0; i < num_fps; i++)
				fprintf(fps[i], "%lx,", addr);

			// Writes data for each thread
			for (const auto & t_it : tid_index) {
				auto l = get_latencies_from_cell(addr, t_it.first);

				// No data
				if (l.empty()) {
					for (size_t j = 0; j < num_fps; j++)
						fprintf(fps[j], "-1,");
					continue;
				}

				accesses[t_it.second] += l.size();

				// Calculates data
				const auto num_accesses = l.size();
				const auto min_latency = *(std::min_element(l.begin(), l.end()));
				const double mean_latency = std::accumulate(l.begin(), l.end(), 0.0) / num_accesses;
				const auto max_latency = *(std::max_element(l.begin(), l.end()));

				// Prints data to files
				fprintf(fps[0], "%lu,", num_accesses);
				fprintf(fps[1], "%lu,", min_latency);
				fprintf(fps[2], "%.2f,", mean_latency);
				fprintf(fps[3], "%lu,", max_latency);
			}

			for (size_t j = 0; j < num_fps; j++)
				fprintf(fps[j], "\n");
		}

		// After all addresses (rows) are processed, we print number of accesses by each thread in the last row
		for (size_t i = 0; i < num_fps; i++) {
			fprintf(fps[i], "num_accesses,"); // Rowname
			for (size_t j = 0; j < tid_index.size(); j++)
				fprintf(fps[i], "%lu,", accesses[j]);
		}

		delete[] accesses;
	}

	void print_alt_graph (FILE * fp) {
		// First line of the CSV file: column header
		fprintf(fp, "addr,threads_accessed\n");

		// Each unique page address will write a row with the data for each thread
		for (const auto & addr : uniq_addrs){
			int threads_accessed = 0;

			// A thread has accessed to the page if its cell is not null
			for (const auto & t_it : tid_index)
				threads_accessed += ( get_cell(addr, t_it.first) != NULL );

			// Prints row (row name and data) to file
			fprintf(fp, "%lx,%d\n", addr, threads_accessed);
		}
	}

	friend std::ostream & operator << (std::ostream & os, const page_table_t & pg) {
		os << "Page table for PID: " << pg.pid << '\n';

		// Prints each row (TID)
		for (const auto & t_it : pg.tid_index) {
			pid_t tid = t_it.first;
			int pos = t_it.second;

			os << "TID: " << tid << '\n';
			if (pg.table[pos].empty())
				os << "\tEmpty.\n";

			// Prints each cell (TID + page address)
			for (const auto & it : pg.table[pos]) {
				const auto page_addr = it.first;
				const auto cell = it.second;
				const auto current_node = pg.page_node_map.at(page_addr).current_node;

				os << "\tPAGE_ADDR: " << page_addr << "x, CURRENT_NODE: " << current_node << ", " << cell << '\n';
			}
			os << '\n';
		}
		return os;
	}
};

#endif
