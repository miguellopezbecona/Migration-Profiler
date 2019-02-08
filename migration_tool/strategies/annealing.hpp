#pragma once
#ifndef __ANNEALING_STRATEGY__
#define __ANNEALING_STRATEGY__

#include <iostream>
#include <vector>

#include "types_definition.hpp"

#include "strategy.hpp"
#include "migration/migration_cell.hpp"

const int TICKETS_MEM_CELL_WORSE[] = {1, 1};
const int TICKETS_MEM_CELL_NO_DATA[] = {2, 2};
const int TICKETS_MEM_CELL_BETTER[] = {4, 4};
const int TICKETS_FREE_CORE = 3;

double last_performance = -1;
// labeled_migr_t last_migration; // Always with 1 or 2 elements

#define ANNEALING_PRINT
//// Heavily based on what Óscar implemented in his PhD

// Special structure which takes into account that each possible choice is a simple migration or an interchange, and has tickets associated to it
class labeled_migr_t {
public:
	std::vector<migration_cell_t> potential_migr; // 1 element for simple migration, 2 for interchange
	int tickets;

	static labeled_migr_t last_migration; // Always with 1 or 2 elements

public:
	labeled_migr_t () :
		potential_migr(),
		tickets(0)
	{};

	labeled_migr_t (const migration_cell_t mc, const int t) :
		potential_migr(),
		tickets(t)
	{
		potential_migr.push_back(mc);
	};

	labeled_migr_t (const migration_cell_t mc1, const migration_cell_t mc2, const int t) :
		potential_migr(),
		tickets(t)
	{
		potential_migr.push_back(mc1);
		potential_migr.push_back(mc2);
	}

	inline bool is_interchange () const {
		return potential_migr.size() == 2;
	}

	void prepare_for_undo () {
		if (!is_interchange())
			potential_migr[0].interchange_dest();
		else {
			potential_migr[0].prev_dest = potential_migr[0].dest;
			potential_migr[1].prev_dest = potential_migr[1].dest;

			potential_migr[0].dest = potential_migr[1].prev_dest;
			potential_migr[1].dest = potential_migr[0].prev_dest;
		}
	}

	/*** Rest of functions ***/
	static std::vector<migration_cell_t> undo_last_migration () {
		last_migration.prepare_for_undo();

		return last_migration.potential_migr;
	}

	inline void print () const {
		std::cout << *this << '\n';
	}

	friend std::ostream & operator << (std::ostream & os, const labeled_migr_t & m) {
		const char* const choices[] = {" simple migration", "n interchange"};
		os << "It is a" << choices[m.is_interchange()] << ". " << m.tickets << " tickets:" << '\n';
		for (migration_cell_t const & mc : m.potential_migr) {
			os << "\t" << mc << '\n';
		}
		return os;
	}
};

labeled_migr_t labeled_migr_t::last_migration;

class annealing_t : public strategy {
private:
	// Gets a different number of tickets depending on perf values and if we are comparing a external thread for interchange or not (mod)
	inline int get_tickets_from_perfs (const size_t mem_cell, const size_t current_cell, const std::vector<double> & perfs, const bool mod) const {
		if (perfs.size() < mem_cell || perfs.size() < current_cell)
			return TICKETS_MEM_CELL_NO_DATA[mod];
		else if (perfs[mem_cell] == PERFORMANCE_INVALID_VALUE)
			return TICKETS_MEM_CELL_NO_DATA[mod];
		else if (perfs[current_cell] > perfs[mem_cell])
			return TICKETS_MEM_CELL_WORSE[mod];
		else
			return TICKETS_MEM_CELL_BETTER[mod];
	}

	std::vector<labeled_migr_t> get_candidate_list (const tid_t worst_tid, const page_table_t & page_t) const {
		std::vector<labeled_migr_t> migration_list;

		const auto current_cpu = system_struct_t::get_cpu_from_tid(worst_tid);
		const auto current_cell = system_struct_t::get_cpu_memory_node(current_cpu);
		const auto current_perfs = page_t.get_perf_data(worst_tid);

		// Search potential core destinations from different memory nodes
		for (size_t n = 0; n < system_struct_t::NUM_OF_MEMORIES; n++) {
			if (n == size_t(current_cell))
				continue;

			for (size_t i = 0; i < system_struct_t::CPUS_PER_MEMORY; i++) {
				const auto actual_cpu = system_struct_t::node_cpu_map[n][i];

				auto tickets = get_tickets_from_perfs(n, current_cell, current_perfs, false);

				const migration_cell_t mc(worst_tid, actual_cpu, current_cpu, page_t.pid, true); // Migration associated to this iteration (CPU)

				// Free core: posible simple migration with a determined score
				if (system_struct_t::is_cpu_free(actual_cpu)) {
					tickets += TICKETS_FREE_CORE;

					labeled_migr_t lm(mc, tickets);
	 				migration_list.push_back(lm);
					continue;
				}

				// We will choose the TID with generates the higher number of tickets
				const auto tids = system_struct_t::get_tids_from_cpu(actual_cpu);
				tid_t other_tid = -1;
				int aux_tickets = -1;

				for (pid_t const & aux_tid : tids) {
					const auto other_perfs = page_t.get_perf_data(aux_tid);
					int tid_tickets = get_tickets_from_perfs(current_cell, n, other_perfs, true);

					// Better TID to pick
					if (tid_tickets > aux_tickets) {
						aux_tickets = tid_tickets;
						other_tid = aux_tid;
					}
				}

				tickets += aux_tickets;

				const migration_cell_t mc2(other_tid, current_cpu, actual_cpu, page_t.pid, true);
				const labeled_migr_t lm(mc, mc2, tickets);
				migration_list.push_back(lm);
			}
		}

		return migration_list;
	}

	// Picks migration or interchange from candidate list
	labeled_migr_t get_random_labeled_cell(const std::vector<labeled_migr_t> & lm_list) const {
		int total_tickets = 0;
		for (labeled_migr_t const & lm : lm_list)
			total_tickets += lm.tickets;
		auto result = rand() % total_tickets; // Gets random number up to total tickets

		for (labeled_migr_t const & lm : lm_list) {
			result -= lm.tickets; // Subtracts until lower than zero
			if (result < 0)
				return lm;
		}

		// Should never reach here
		return lm_list[0];
	}

	std::vector<migration_cell_t> get_iteration_migration (page_table_t & page_t) const {
		pid_t worst_tid = page_t.normalize_perf_and_get_worst_thread();

		#ifdef ANNEALING_PRINT
		std::cout << "\n*\nWORST THREAD IS: " << worst_tid << '\n';
		#endif

		//page_t.print_performance(); // Perfs after normalization

		// Selects migration targets for lottery (this is where the algoritm really is)
		const auto migration_list = get_candidate_list(worst_tid, page_t);

		#ifdef ANNEALING_PRINT
		std::cout << "\n*\nMIGRATION LIST CONTENT:" << '\n';
		for (labeled_migr_t const & lm : migration_list)
			std::cout << lm;
		#endif

		// I think this will only happen in no-NUMA systems, for testing
		if (migration_list.empty()) {
			#ifdef ANNEALING_PRINT
			std::cout << "TARGET LIST IS EMPTY, NO MIGRATIONS" << '\n';
			#endif
			std::vector<migration_cell_t> emp;
			return emp;
		}

		const auto target_cell = get_random_labeled_cell(migration_list);

		#ifdef ANNEALING_PRINT
		std::cout << "\n*\nTARGET MIGRATION: " << target_cell;
		#endif

		// Saving for possible undo, prepared latter
		labeled_migr_t::last_migration = target_cell;

		return target_cell.potential_migr;
	}

	std::vector<migration_cell_t> get_iteration_migration (std::map<pid_t, page_table_t> & page_ts) const {
		tid_t worst_tid = -1;
		double min_p = 1e15;

		// normalize_perf_and_get_worst_thread() for all tables
		for (auto & t_it : page_ts) {
			auto & t = t_it.second;
			auto local_worst_t = t.normalize_perf_and_get_worst_thread();

			// No active threads for that PID
			if (local_worst_t == -1)
				continue;

			auto pd = t.perf_per_tid[local_worst_t];
			auto local_worst_p = pd.v_perfs[pd.index_last_node_calc];

			if (local_worst_p < min_p) {
				worst_tid = local_worst_t;
				min_p = local_worst_p;
			}
		}

		// No active threads
		if (worst_tid == -1) {
			#ifdef ANNEALING_PRINT
			std::cout << "NO SUITABLE THREADS, NO MIGRATIONS" << '\n';
			#endif
			std::vector<migration_cell_t> emp;
			return emp;
		}

		#ifdef ANNEALING_PRINT
		std::cout.precision(2); std::cout << std::fixed;
		std::cout << "WORST THREAD IS: " << worst_tid << ", WITH PERF: " << min_p << '\n';
		std::cout << std::defaultfloat;
		#endif

		//for (auto & t_it : page_ts) // Perfs after normalization
			//t_it.second.print_performance();

		// Selects migration targets for lottery (this is where the algoritm really is)
		const auto migration_list = get_candidate_list(worst_tid, page_ts);

		#ifdef ANNEALING_PRINT
		// Commented due to its high amount of printing in manycores
		// std::cout << "MIGRATION LIST CONTENT:";
		// for(labeled_migr_t const & lm : migration_list){
		// 	std::cout << "\t" << lm;
		// }
		#endif

		// I think this will only happen in no-NUMA systems, for testing
		if (migration_list.empty()) {
			#ifdef ANNEALING_PRINT
			std::cout << "TARGET LIST IS EMPTY, NO MIGRATIONS" << '\n';
			#endif
			return std::vector<migration_cell_t>();
		}

		const auto target_cell = get_random_labeled_cell(migration_list);

		#ifdef ANNEALING_PRINT
		std::cout << "TARGET MIGRATION: " << target_cell;
		#endif

		// Saving for possible undo, prepared latter
		labeled_migr_t::last_migration = target_cell;

		return target_cell.potential_migr;
	}

	/*** Functions for global version ***/
	std::vector<labeled_migr_t> get_candidate_list (const pid_t worst_tid, const std::map<pid_t, page_table_t> & page_ts) const {
		std::vector<labeled_migr_t> migration_list;

		const auto current_cpu = system_struct_t::get_cpu_from_tid(worst_tid);
		const auto current_cell = system_struct_t::get_cpu_memory_node(current_cpu);

		const auto current_pid = system_struct_t::get_pid_from_tid(worst_tid);
		const auto current_perfs = page_ts.at(current_pid).get_perf_data(worst_tid);

		// Search potential core destinations from different memory nodes
		for (size_t n = 0; n < system_struct_t::NUM_OF_MEMORIES; n++) {
			if (n == size_t(current_cell))
				continue;

			for (size_t i = 0; i < system_struct_t::CPUS_PER_MEMORY; i++) {
				const auto actual_cpu = system_struct_t::node_cpu_map[n][i];
				auto tickets = get_tickets_from_perfs(n, current_cell, current_perfs, false);

				migration_cell_t mc(worst_tid, actual_cpu, current_cpu, current_pid, true); // Migration associated to this iteration (CPU)

				// Free core: posible simple migration with a determined score
				if (system_struct_t::is_cpu_free(actual_cpu)) {
					tickets += TICKETS_FREE_CORE;

					labeled_migr_t lm(mc, tickets);
	 				migration_list.push_back(lm);
					continue;
				}

				// Not a free core: get its TIDs info so a possible interchange can be planned

				// We will choose the TID with generates the higher number of tickets
				const auto tids = system_struct_t::get_tids_from_cpu(actual_cpu);
				tid_t other_tid = -1;
				int aux_tickets = -1;

				for (const auto & aux_tid : tids) {
					const auto other_pid = system_struct_t::get_pid_from_tid(aux_tid);
					const auto other_perfs = (page_ts.find(other_pid) != page_ts.end()) ?
						page_ts.at(other_pid).get_perf_data(aux_tid) : std::vector<double>();
					const auto tid_tickets = get_tickets_from_perfs(current_cell, n, other_perfs, true);

					// Better TID to pick
					if (tid_tickets > aux_tickets) {
						aux_tickets = tid_tickets;
						other_tid = aux_tid;
					}
				}

				tickets += aux_tickets;

				migration_cell_t mc2(other_tid, current_cpu, actual_cpu, system_struct_t::get_pid_from_tid(other_tid), true);
				labeled_migr_t lm(mc, mc2, tickets);
				migration_list.push_back(lm);
			}
		}

		return migration_list;
	}

public:
	// Only valid for threads
	// One PID version
	std::vector<migration_cell_t> get_threads_to_migrate (page_table_t & page_t) const {
		std::vector<migration_cell_t> ret;

		const auto current_performance = page_t.get_total_performance();
		const auto diff = current_performance / last_performance;

		#ifdef TH_MIGR_OUTPUT
		std::cout.precision(2); std::cout << std::fixed;
		std::cout << "\nCurrent Perf: " << current_performance << ". Last Perf: " << last_performance << ". Ratio: " << diff << ". SBM: " << get_time_value() << '\n';
		std::cout << std::defaultfloat;
		#endif

		last_performance = current_performance;

		if (diff < 0) // First time or no data, so we do nothing
			last_performance = current_performance; // Redundant, but we need an useless sentence
		else if (diff < 0.9) { // We are doing MUCH worse, let's undo
			time_go_up();
			page_t.reset_performance();

			return labeled_migr_t::undo_last_migration();
		} else if (diff < 1.0) // We are doing better or equal
			time_go_up();
		else // We are doing better or equal
			time_go_down();

		#ifdef ANNEALING_PRINT
		std::cout << "\n*\nPERFORMING MIGRATION ALGORITHM" << '\n';
		#endif

		ret = get_iteration_migration(page_t);

		// Cleans performance data and finishes iteration
		page_t.reset_performance();

		return ret;
	}

	std::vector<migration_cell_t> get_threads_to_migrate (std::map<pid_t, page_table_t> & page_ts) const {
		std::vector<migration_cell_t> ret;

		double current_performance = 0.0;

		for (const auto & t_it : page_ts)
			current_performance += t_it.second.get_total_performance();

		// No performance obtained, so no suitable threads
		if (current_performance < 1e-6)
			return ret;

		double diff = current_performance / last_performance;

		#ifdef TH_MIGR_OUTPUT
		std::cout.precision(2); std::cout << std::fixed;
		std::cout << "\n*\nCurrent Perf: " << current_performance << ". Last Perf: " << last_performance << ". Ratio: " << diff << ". SBM: " << get_time_value() << '\n';
		std::cout << std::defaultfloat;
		#endif

		last_performance = current_performance;

		if (diff < 0) // First time or no data, so we do nothing
			last_performance = current_performance; // Redundant, but we need an useless sentence
		else if (diff < 0.9) { // We are doing MUCH worse, let's undo
			time_go_up();

			for (auto & t_it : page_ts) {
				t_it.second.reset_performance();
			}

			return labeled_migr_t::undo_last_migration();
		} else if (diff < 1.0) // We are doing a bit worse, let's migrate more often
			time_go_up();
		else // We are doing better or equal, let's migrate less often
			time_go_down();

		#ifdef ANNEALING_PRINT
		std::cout << "PERFORMING MIGRATION ALGORITHM" << '\n';
		#endif

		ret = get_iteration_migration(page_ts);

		// Cleans performance data and finishes iteration
		for (auto & t_it : page_ts)
			t_it.second.reset_performance();

		return ret;
	}
};

#endif
