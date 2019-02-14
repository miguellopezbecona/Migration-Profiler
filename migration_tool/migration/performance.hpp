#pragma once
#ifndef __PERFORMANCE__
#define __PERFORMANCE__

#include <iostream>

#include <vector>
#include <map>

#include "system_struct.hpp"

// Might be used in the future to ponderate performance calculation. Not used right now
const int DYRM_ALPHA = 1;
const int DYRM_BETA = 1;
const int DYRM_GAMMA = 1;

const int PERFORMANCE_INVALID_VALUE = -1;

// Base perf data for threads and memory pages. Now is mainly used for storing page locations. It may be more relevant in the future
class base_perf_data_t {
public:
	unsigned short num_uniq_accesses;
	unsigned short num_acs_thres; // Always equal or lower than num_uniq_accesses
	lat_t min_latency;
	lat_t median_latency;
	lat_t max_latency;

	base_perf_data_t () {}

	inline void print () const {
		std::cout << *this << '\n';
	}

	friend std::ostream & operator << (std::ostream & os, const base_perf_data_t & b) {
		if (b.num_acs_thres > 0)
			os << "UNIQ_ACS: " << b.num_uniq_accesses << ", ACS_THRES: " << b.num_acs_thres << ", MIN_LAT: " << b.min_latency << ", MEDIAN_LAT: " << b.median_latency << ", MAX_LAT: " << b.max_latency;
		return os;
	}
};

class pg_perf_data_t : public base_perf_data_t {
public:
	node_t current_node;
	cpu_t last_cpu_access;

	std::vector<unsigned short> acs_per_node; // Number of accesses from threads per memory node

	pg_perf_data_t () :
		acs_per_node(system_struct::NUM_OF_MEMORIES, 0)
	{
		num_acs_thres = 0;
	}

	inline void print () const {
		std::cout << *this << '\n';
	}

	friend std::ostream & operator << (std::ostream & os, const pg_perf_data_t & pg) {
		if (pg.num_acs_thres > 0) {
			os << "UNIQ_ACS: " << pg.num_uniq_accesses << ", ACS_THRES: " << pg.num_acs_thres << ", MIN_LAT: " << pg.min_latency << ", MEDIAN_LAT: " << pg.median_latency << ", MAX_LAT: " << pg.max_latency << '\n';
		}

		os << "MEM_NODE: " << pg.current_node << ", ACS_PER_NODE: {";
		for (const auto & elem : pg.acs_per_node) {
			os << " " << elem;
		}
		os << " }" << '\n';
		return os;
	}
};

class th_perf_data_t : base_perf_data_t {};

// Based on Óscar's work in his PhD. This would be associated to each thread from the PID associated to the table
class rm3d_data_t {
public:
	const size_t CACHE_LINE_SIZE = 64;

	bool active; // A TID is considered active if we received samples from it in the current iteration

	// Sum of inst sample fields, per core
	std::vector<ins_t> insts;
	std::vector<req_t> reqs;
	std::vector<tim_t> times;

	std::vector<double> v_perfs; // 3DyRM performance per memory node
	unsigned char index_last_node_calc; // Used to get last_3DyRM_perf from Óscar's code

	rm3d_data_t () :
		active(false),
		insts(system_struct::NUM_OF_CPUS, 0),
		reqs(system_struct::NUM_OF_CPUS, 0),
		times(system_struct::NUM_OF_CPUS, 0),
		v_perfs(system_struct::NUM_OF_MEMORIES, PERFORMANCE_INVALID_VALUE)
	{
		// reset();
	}

	void inline add_data (const cpu_t cpu, const ins_t inst, const req_t req, const tim_t time) {
		active = true;

		insts[cpu] += inst;
		reqs[cpu] += req;
		times[cpu] += time;
	}

	void calc_perf (const double mean_lat) {
		const double inv_mean_lat = 1.0 / mean_lat;

		for (size_t cpu = 0; cpu < system_struct::NUM_OF_CPUS; cpu++) {
			if (reqs[cpu] == 0) // No data, bye
				continue;

			// Divide by zeros check should be made. It's assumed it won't happen if the thread is inactive
			const double inst_per_s = (double) insts[cpu] * 1000.0 / times[cpu]; // Óscar used inst/ms, so * 10^3
			const double inst_per_b = (double) insts[cpu] / (reqs[cpu] * CACHE_LINE_SIZE);

			const auto cpu_node = system_struct::get_cpu_memory_node(cpu);
			v_perfs[cpu_node] = inv_mean_lat * inst_per_s * inst_per_b;

			// Debug
			//printf("PERF: %.2f, MEAN_LAT = %.2f, INST_S = %.2f, INST_B = %.2f\n", v_perfs[cpu_node], mean_lat, inst_per_s, inst_per_b);

			index_last_node_calc = cpu_node;
		}
	}

	void inline reset () {
		active = false;

		fill(insts.begin(), insts.end(), 0);
		fill(reqs.begin(), reqs.end(), 0);
		fill(times.begin(), times.end(), 0);
		fill(v_perfs.begin(), v_perfs.end(), PERFORMANCE_INVALID_VALUE);
	}

	void inline print () const {
		std::cout << *this << '\n';
	}

	friend std::ostream & operator << (std::ostream & os, const rm3d_data_t & d) {
		for (size_t cpu = 0; cpu < system_struct::NUM_OF_CPUS; cpu++) {
			os << "\tCPU: " << cpu << ", INSTS = " << d.insts[cpu] << ", REQS = " << d.reqs[cpu] << ", TIMES = " << d.times[cpu];
		}

		os.precision(2); os << std::fixed;
		for (size_t node = 0; node < system_struct::NUM_OF_MEMORIES; node++) {
			os << "\tNODE: " << node << ", PERF = " << d.v_perfs[node] << '\n';
		}
		os << std::defaultfloat;
		return os;
	}
};

/*** Experimental structures ***/

// The content of each cell of these new tables
class perf_cell_t {
public:
	// Right now, we just have into account the number of acceses and the mean latency
	size_t num_acs;
	double mean_lat;

	perf_cell_t () :
		num_acs(0),
		mean_lat(0.0)
	{}

	perf_cell_t (const double lat) :
		num_acs(1),
		mean_lat(lat)
	{}

	void update_mlat (const double lat) {
		mean_lat = (mean_lat * num_acs + lat) / (num_acs + 1);
		num_acs++;
	}

	inline bool is_filled () const {
		return num_acs > 0;
	}

	inline void print () const {
		std::cout << *this << '\n';
	}

	friend std::ostream & operator << (std::ostream & os, const perf_cell_t & p) {
		if (!p.is_filled())
			os << "Not filled.";
		else {
			os.precision(2); os << std::fixed;
			os << "NUM_ACS: " << p.num_acs << ", MEAN_LAT: " << p.mean_lat << '\n';
			os << std::defaultfloat;
		}
		return os;
	}
};

// Experimental table to store historical performance for each thread/memory page in each CPU/node
using perf_col = std::vector<perf_cell_t>; // Readibility

class perf_table_t {
public:
	static double alfa; // For future aging technique

	unsigned short coln; // NUM_OF_MEMORIES or NUM_OF_CPUS
	std::map<addr_t, perf_col> table; // Dimensions: TID/page addr and CPU/node

	perf_table_t () :
		coln(1),
		table()
	{}

	perf_table_t (const size_t n) :
		coln(n),
		table()
	{}

	inline bool has_row (const addr_t key) const {
		return table.count(key) > 0;
	}

	void remove_row (const addr_t key) {
		if (!has_row(key))
			return;

		table[key].clear();
		table.erase(key);
	}

	void add_data (const addr_t key, const size_t col_num, const double lat) {
		if (!has_row(key)) { // No entry, so we create it
			std::vector<perf_cell_t> pv(coln);
			table[key] = pv;

			for (size_t i = 0; i < coln; i++) {
				perf_cell_t pc;
				table[key][i] = pc;
			}
		}

		table[key][col_num].update_mlat(lat); // In any case, we update the mean
	}

	inline void print () const {
		std::cout << *this << '\n';
	}

	friend std::ostream & operator << (std::ostream & os, const perf_table_t & p) {
		const char* const keys[] = {"TID", "Page address"};
		const char* const colns[] = {"CPU", "Node"};
		bool is_page_table = (p.coln == system_struct::NUM_OF_MEMORIES);

		os << "Printing perf table (" << keys[is_page_table] << "/" << colns[is_page_table] << " type)" << '\n';

		// Prints each row
		for (const auto & it : p.table) {
			const auto & row = it.first;
			const auto & pv = it.second;

			os << keys[is_page_table] << ": " << row << '\n';

			// Prints each non-empty cell
			for (size_t i = 0; i < p.coln; i++) {
				if (!pv[i].is_filled()) {
					continue;
				}

				os << "\t" << colns[is_page_table] << " " << i << " " << pv[i];
			}
		}
		return os;
	}

	// Some future functions could be coded such as: how many CPUs have data for a given TID?, which CPU has the most/least accesses?, etc.
};

double perf_table_t::alfa = 1.0;
perf_table_t tid_cpu_table;

#endif
