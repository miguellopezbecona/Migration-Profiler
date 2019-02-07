#pragma once
#ifndef __GENETIC_STRATEGY__
#define __GENETIC_STRATEGY__

#include <iostream>
#include <iomanip>

#include "strategy.hpp"

#include "genetic/population.hpp"
#include "genetic/gen_utils.hpp"
#include "migration/migration_cell.hpp"

const double CROSS_PROB = 0.5;
const double MUT_PROB = 0.1;

class genetic_t : public strategy {
public:
	population p;
	size_t it = 0;
	size_t best_it = 0;
	individual_t best_sol;

	genetic_t () :
		p(),
		it(0),
		best_it(0),
		best_sol()
	{};

	std::vector<migration_cell_t> do_genetic (const individual_t & ind) {
		std::vector<migration_cell_t> v; // Holds migrations generated by genetic processes

		it++;

		#ifdef GENETIC_OUTPUT
		std::cout << "Beginning of iteration " << it << " of genetic process." << '\n';
		std::cout << "Current idv is: " << ind << '\n';
		#endif

		/** Tournament, cross, mutation, and replacement **/

		// If there are no other individuals, only mutation will be done
		individual_t child = ind;
		if (!p.is_first_iteration()) {
			p.update_fitness(ind.get_fitness());

			individual_t chosen = tournament();
			child = cross(ind, chosen, v);
		}
		mutation(child, v);
		p.add(child); // Final result is appended to population with replacement

		#ifdef GENETIC_OUTPUT
		std::cout << "End of iteration " << it << ". ";
		std::cout << p << '\n'; // Prints poblation content
		#endif

		const individual_t best = p.get_best_ind();

		// Updates best_sol if needed
		#ifdef MAXIMIZATION
		if (best.get_fitness() > best_sol.get_fitness()) {
		#elif defined(MINIMIZATION)
		if (best.get_fitness() < best_sol.get_fitness()) {
		#endif
			best_sol = best;
			best_it = it;
		}


		#ifdef GENETIC_OUTPUT
		std::cout << '\n';
		#endif

		return v;
	}

	individual_t tournament () const {
		const individual_t ind = p.get_best_ind();

		#ifdef GENETIC_OUTPUT
		std::cout << "Winner idv. from selection: " << ind << '\n';
		#endif

		return ind;
	}

	individual_t cross (individual_t from_iter, individual_t from_selec, std::vector<migration_cell_t> & v) {
		const auto sz = from_iter.size();

		// Cross probability
		auto prob = gen_utils::get_rand_double();

		#ifdef GENETIC_OUTPUT
		std::cout.precision(2); std::cout << std::fixed;
		std::cout << "Cross: (rand num: " << prob << ") -> ";
		std::cout << std::defaultfloat;
		#endif

		if (prob > CROSS_PROB) {
			#ifdef GENETIC_OUTPUT
			std::cout << "cross not done" << '\n';
			#endif
			return from_iter;
		} else {
			const auto idx1 = gen_utils::get_rand_int(sz, -1); // Any possible index
			const auto idx2 = gen_utils::get_rand_int(sz, idx1); // The second index can't be the first one...

			#ifdef GENETIC_OUTPUT
			std::cout << "index cuts: " << idx1 << " " << idx2 << '\n';
			#endif

			// Two descendents can be obtained with order crossover
			individual_t c1 = from_selec.cross(from_iter, idx1, idx2);
			individual_t c2 = from_iter.cross(from_selec, idx1, idx2);

			// We can only keep one child, decided randomly
			prob = gen_utils::get_rand_double();
			individual_t chosen_child;
			if (prob > 0.5) {
				#ifdef GENETIC_OUTPUT
				std::cout << "\tWe kept first child: ";
				#endif
				chosen_child = c1;
			} else {
				#ifdef GENETIC_OUTPUT
				std::cout << "\tWe kept second child: " << '\n';
				#endif
				chosen_child = c2;
			}

			#ifdef GENETIC_OUTPUT
			std::cout << chosen_child << '\n';
			#endif

			// We generate migration cells seeing the differences between the idvs from the iteration and the selected son
			for (size_t i = 0; i < sz; i++) {
				if (from_iter[i] != chosen_child[i]) { // Difference: TIDs from child are migrated to its CPU possition
					for (pid_t tid : chosen_child[i]) {
						const migration_cell_t mc(tid, system_struct_t::ordered_cpus[i]);
						v.push_back(mc);
					}
				}
			}

			return chosen_child;
		}
	}

	void mutation (individual_t & ind, std::vector<migration_cell_t> & v) {
		#ifdef GENETIC_OUTPUT
		std::cout << "Mutation process:" << '\n';
		#endif

		const auto sz = ind.size();
		for (size_t pos1 = 0; pos1 < sz; pos1++) {

			// Obtains probability
			const auto prob = gen_utils::get_rand_double();

			#ifdef GENETIC_OUTPUT
			std::cout.precision(2); std::cout << std::fixed;
			std::cout << "\tIndex: " << pos1 << " (rand rum " << prob << ") -> ";
			std::cout << std::defaultfloat;
			#endif

			if (prob > MUT_PROB) {
				#ifdef GENETIC_OUTPUT
				std::cout << "mutation not done" << '\n';
				#endif
				continue;
			}

			// Mutation by interchange: selects second index
			const auto pos2 = gen_utils::get_rand_int(sz, pos1);

			// Generates two migration cells: ind[pos1] goes to ind[pos2] location and viceversa
			const auto dest1 = system_struct_t::ordered_cpus[pos2]; // CPU associated to pos2
			const auto dest2 = system_struct_t::ordered_cpus[pos1];

			// Migration cells concerning to those TIDs may already exist from cross process. If this is the case, we just have to change their destination
			bool first_exists = false, sec_exists = false;
			for (migration_cell_t & mc : v) {
				for (const auto & tid : ind.v[pos1]) {
					if (mc.elem == size_t(tid)) { // Cell already exists: changes its destination
						mc.dest = dest1;
						first_exists = true;
					}
				}

				for (const auto & tid : ind.v[pos2]) {
					if (mc.elem == size_t(tid)) { // Cell already exists: changes its destination
						mc.dest = dest2;
						sec_exists = true;
					}
				}
			}

			// It won't generate migration cells for free CPUs positions (empty arrays) or existing cells
			if (!first_exists) {
				for (pid_t const & tid : ind.v[pos1]) {
					const migration_cell_t mc1(tid, dest1);
					v.push_back(mc1);
				}
			}
			if (!sec_exists) {
				for (pid_t const & tid : ind.v[pos2]) {
					const migration_cell_t mc2(tid, dest2);
					v.push_back(mc2);
				}
			}

			// Key function
			ind.mutate(pos1, pos2);

			#ifdef GENETIC_OUTPUT
			std::cout << "interchanges with position " << pos2 << '\n';
			#endif
		}
	}

	void print_best_sol () const {
		std::cout << "BEST SOLUTION: " << best_sol << '\n';
		std::cout << "Obtained in iteration: " << best_it << '\n';
	}

	// The following should not be written because it should inherit it from strategy
	inline std::vector<migration_cell_t> get_pages_to_migrate (const std::map<pid_t, page_table_t> & page_ts) {
		// Generates an individual for current state and calculates its performance
		return do_genetic(individual_t(page_ts));
	}

	inline std::vector<migration_cell_t> get_threads_to_migrate (const std::map<pid_t, page_table_t> & page_ts) {
		// Right now, for simplicity:
		return get_pages_to_migrate(page_ts);
	}
};

#endif
