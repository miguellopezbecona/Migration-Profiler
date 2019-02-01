#pragma once
#ifndef __INDIVIDUAL__
#define __INDIVIDUAL__

#include "strategies/genetic/gen_utils.hpp" // MAX/MIN macro
#include "migration/page_table.hpp" // It uses it in one constructor

// This would fit better in genetic.h, but it raised double link problems
#define GENETIC_OUTPUT

typedef std::vector<pid_t> gene; // List of TID per CPU

#ifdef MAXIMIZATION
const double NO_FITNESS = -1.0;
#elif defined(MINIMIZATION)
const double NO_FITNESS = 1e6;
#endif

class individual_t {
public:
	std::vector<gene> v; // Internal representation: list of TIDs where the index indicates the (ordered) CPU
	double fitness; // Currently defined as the mean latency of all the system. The lower the better

	individual_t () :
		v(),
		fitness(NO_FITNESS)
	{}

	individual_t (const std::map<pid_t, page_table_t> & ts) :
		v(system_struct_t::NUM_OF_CPUS),
		fitness(NO_FITNESS)
	{
		// Builds list from system_struct data
		for (int i = 0; i < system_struct_t::NUM_OF_CPUS; i++){
			if (system_struct_t::is_cpu_free(i))
				continue;

			// Correct index of CPU in ordered_cpus
			const auto index = find(system_struct_t::ordered_cpus.begin(), system_struct_t::ordered_cpus.end(), i) - system_struct_t::ordered_cpus.begin();

			// Assigns list
			v[index] = system_struct_t::get_tids_from_cpu(i);
		}

		// Calculates fitness (in this case, mean latency), using only data from last iteration
		std::vector<int> all_ls;
		for (const auto & it : ts) { // For each table (PID), gets all its latencies
			const std::vector<int> l = it.second.get_all_lats();
			all_ls.insert(end(all_ls), begin(l), end(l));
		}

		// After all_ls is filled, the mean is calculated:
		this->fitness = std::accumulate(all_ls.begin(), all_ls.end(), 0.0) / all_ls.size();
	}

	individual_t (const std::vector<gene> & vec) :
		v(vec),
		fitness(NO_FITNESS)
	{}

	inline double get_fitness () const {
		return fitness;
	}

	inline size_t size () const {
		return v.size();
	}

	inline void set (const int index, const gene value) {
		v[index] = value;
	}

	inline bool is_too_old () const {
		const double PERC_THRES = 0.5;

		unsigned short num_tids = 0;
		unsigned short dead_tids = 0;

		// Check the percentage of dead TIDs the idv has
		for (gene const & tids : v) {
			for (pid_t const & tid : tids) {
				num_tids++;

				pid_t pid = system_struct_t::get_pid_from_tid(tid);
				if (!is_tid_alive(pid,tid))
					dead_tids++;
			}
		}

		double perc = dead_tids / num_tids;
		return perc >= PERC_THRES;
	}

	inline void mutate (const size_t idx1, const size_t idx2) { // Interchange
		std::swap(v[idx1], v[idx2]);
	}

	individual_t cross (const individual_t & other, const int idx1, const int idx2) const { // Using order crossover
		const size_t sz = v.size();
		int cut1, cut2, copy_idx;

		gene num;

		// Gets which one is the first cut and which is the second
		if (idx1 > idx2) {
			cut1 = idx2;
			cut2 = idx1;
		} else {
			cut1 = idx1;
			cut2 = idx2;
		}

		// We will only allow a maximum of free CPUs (repeated values)
		gene empty; // Empty list -> free CPU
		short free_cpus = count(v.begin(), v.end(), empty);
		short free_cpus_other = count(other.v.begin(), other.v.end(), empty);
		if (free_cpus_other > free_cpus)
			free_cpus = free_cpus_other;

		// Creates a copy which will already have the center block copied
		individual_t son = get_copy();

		// Gets central block from current individual
		std::set<gene> sublist(v.begin() + cut1, v.begin() + cut2 + 1);

		//// Changes what is not the central block ////

		// Copies from second cut until the end from the other individual
		copy_idx = (cut2+1) % sz;
		for (size_t i = cut2 + 1; i < sz; i++) {
			// If the generated index is already part of the sublist, then we continue to the next in a circular manner
			do {
				num = other[copy_idx];
				copy_idx = (copy_idx + 1) % sz;

				// We will only allow a maximum of free CPUs (repeated values)
				if(num.empty() && free_cpus){
					free_cpus--;
					break;
				}
			} while(sublist.count(num)); // sublist.contains(num)
			sublist.insert(num);
			son.set(i, num);
		}

		// Copies from start to second cut
		for (int i = 0; i < cut1; i++) {
			do {
				num = other[copy_idx];
				copy_idx++;

				// We will only allow a maximum of free CPUs (repeated values)
				if(num.empty() && free_cpus){
					free_cpus--;
					break;
				}
			} while(sublist.count(num));
			sublist.insert(num);
			son.set(i, num);
		}

		return son;
	}

	inline individual_t get_copy() const {
		individual_t i(v);
		return i;
	}

	void print() const {
		std::cout << *this;
	}

	// For easing readibility
	inline gene operator[] (int i) const {
		return v[i];
	}

	inline gene & operator[] (int i) {
		return v[i];
	}

	friend std::ostream & operator << (std::ostream & os, const individual_t & i) {
		os << "{fitness: ";

		if(i.fitness == NO_FITNESS)
			os << "???";
		else {
			os.precision(2); os << std::fixed;
			os << i.fitness;
			os << std::defaultfloat;
		}

		os << ", content: ";

		for (gene const & tids : i.v) {
			if (tids.empty()) {
				os << "F ";  // Free CPU
				continue;
			}

			auto & last = *(--tids.end()); // To know when stop printing underscores
			for (pid_t const & tid : tids) {
				os << tid;
				if (&tid != &last)
					os << "_";
			}
			os << " ";
		}
		os << "}";
		return os;
	}

};

#endif
