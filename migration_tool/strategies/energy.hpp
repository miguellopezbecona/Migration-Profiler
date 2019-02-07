#pragma once
#ifndef __ENERGY_STRATEGY__
#define __ENERGY_STRATEGY__

#include <iostream>
#include <iomanip>

#include <vector>

#include "strategy.hpp"
#include "rapl/energy_data.hpp"
#include "migration/migration_cell.hpp"

#define ENER_OUTPUT

class energy_str_t : public strategy {
public:
	inline std::vector<migration_cell_t> get_migrations (const std::map<pid_t, page_table_t> & page_ts) {
		auto v1 = get_threads_to_migrate(page_ts);
		const auto v2 = get_pages_to_migrate(page_ts);

		#ifdef ENER_OUTPUT
		std::cout << '\n';
		#endif

		v1.insert(v1.end(), v2.begin(), v2.end());
		return v1;
	}

	std::vector<migration_cell_t> get_threads_to_migrate (const std::map<pid_t, page_table_t> & page_ts) {
		const double MAX_PERC_THRES = 0.2; // Threshold maximum increase ratio against base to do migrations
		const double DIFF_PERC_THRES = 0.3; // Minimum ratio difference between lowest and highest to do migrations
		const size_t MAX_THREADS_TO_MIGRATE = 1;
		//const size_t MAX_THREADS_TO_MIGRATE = system_struct_t::CPUS_PER_MEMORY / 4;
		std::vector<migration_cell_t> v;

		// Regarding threads
		auto pkgs = ed.get_curr_vals_from_domain("pkg");
		#ifdef ENER_OUTPUT
		std::cout << "pkg values (minus base) for each node from last iteration: ";
		{
			std::cout.precision(2); std::cout << std::fixed;
			for (double const & c : pkgs) {
				std::cout << c << " ";
			}
			std::cout << std::defaultfloat;
		}
		std::cout << '\n';
		#endif

		const auto it_max = std::max_element(pkgs.begin(), pkgs.end());
		const auto mx = *(it_max);
		const auto from = distance(pkgs.begin(), it_max);

		// If no enough pkg consumption increase -> no thread migrations
		const auto ratio = ed.get_ratio_against_base(mx, from, "pkg");
		#ifdef ENER_OUTPUT
		{
			std::cout.precision(2); std::cout << std::fixed;
			std::cout << "Maximum PKG ratio increase: " << ratio << " (raw: " << mx << "/" << ed.base_vals[from][ed.get_domain_pos("pkg")] << "). Threshold: " << MAX_PERC_THRES << " -> ";
			std::cout << std::defaultfloat;
		}
		#endif

		if (ratio < MAX_PERC_THRES) {
			#ifdef ENER_OUTPUT
			std::cout << "no thread migrations." << "\n\n";
			#endif

			return v;
		}
		#ifdef ENER_OUTPUT
		std::cout << "ok." << '\n';
		#endif

		const auto it_min = std::min_element(pkgs.begin(), pkgs.end());
		const auto mn = *(it_min);

		#ifdef ENER_OUTPUT
		{
			std::cout.precision(2); std::cout << std::fixed;
			std::cout << "Minimum PKG increase: " << mn << ". Diff relation with maximum: " << (mx-mn)/mx << ". Threshold: " << DIFF_PERC_THRES << " -> ";
			std::cout << std::defaultfloat;
		}
		#endif

		// If all nodes are quite memory loaded -> no thread migrations
		if ( ((mx-mn)/mx) < DIFF_PERC_THRES) {
			#ifdef ENER_OUTPUT
			std::cout << "no thread migrations." << "\n\n";
			#endif

			return v;
		}
		#ifdef ENER_OUTPUT
		std::cout << "decided: let's migrate (up to " << MAX_THREADS_TO_MIGRATE << ") pages." << '\n';
		#endif

		// If we meet the criteria, we will migrate from the most loaded node to the least one
		const auto to = distance(pkgs.begin(), it_min);

		#ifdef ENER_OUTPUT
		std::cout << "We will migrate threads from node " << from << " to node " << to << '\n';
		#endif

		// We search TIDs from CPUs from "from" node to be migrated
		// [TODO]: use tid_cpu_table information to decide which threads to migrate
		std::set<int> picked_cpus;
		for (size_t c = 0; c < system_struct_t::CPUS_PER_MEMORY && v.size() < MAX_THREADS_TO_MIGRATE; c++) {
			const auto from_cpu = system_struct_t::node_cpu_map[from][c];

			for (pid_t const & tid : system_struct_t::cpu_tid_map[from_cpu]) {
				if (v.size() == MAX_THREADS_TO_MIGRATE)
					break;

				// We search a, if possible, free CPU to migrate to
				const auto to_cpu = system_struct_t::get_free_cpu_from_node(to, picked_cpus);
				picked_cpus.insert(to_cpu);
				migration_cell_t mc(tid, to_cpu);
				v.push_back(mc);
			}
		}

		#ifdef ENER_OUTPUT
		std::cout << "\n\n";
		#endif

		return v;
	}

	std::vector<migration_cell_t> get_pages_to_migrate (const std::map<pid_t, page_table_t> & page_ts) {
		const double MAX_THRES = 1.0; // Threshold maximum raw increase against base to do migrations
		const double DIFF_THRES = 0.75; // Threshold raw difference between highest and lowest to do migrations
		const size_t MAX_PAGES_TO_MIGRATE = 15;
		std::vector<migration_cell_t> v;

		// Regarding pages
		const auto rams = ed.get_curr_vals_from_domain("ram");
		#ifdef ENER_OUTPUT
		std::cout << "ram values (minus base) for each node from last iteration: ";
		{
			std::cout.precision(2); std::cout << std::fixed;
			for (double const & c : rams)
				std::cout << c << ' ';
			std::cout << std::defaultfloat;
			std::cout << '\n';
		}
		#endif

		// "ram" domain not supported, let's focus on threads
		if (rams.empty())
			return v;

		const auto it_max = max_element(rams.begin(), rams.end());
		const auto mx = *(it_max);

		#ifdef ENER_OUTPUT
		{
			std::cout.precision(2); std::cout << std::fixed;
			std::cout << "Maximum MEMORY increase: " << mx << ". Threshold: " << MAX_THRES << " -> ";
			std::cout << std::defaultfloat;
		}
		#endif

		// No enough memory consumption increase -> no page migrations
		if (mx < MAX_THRES) {
			#ifdef ENER_OUTPUT
			std::cout << "no page migrations." << "\n\n";
			#endif

			return v;
		}
		#ifdef ENER_OUTPUT
		std::cout << "ok." << '\n';
		#endif

		const auto it_min = min_element(rams.begin(), rams.end());
		const auto mn = *(it_min);

		#ifdef ENER_OUTPUT
		{
			std::cout.precision(2); std::cout << std::fixed;
			std::cout << "Minimum MEMORY increase: " << mn << ". Diff with maximum: " << mx - mn << ". Threshold: " << DIFF_THRES << " -> ";
			std::cout << std::defaultfloat;
		}
		#endif

		// If all nodes are quite memory loaded -> no page migrations
		if ( (mx-mn) < DIFF_THRES) {
			#ifdef ENER_OUTPUT
			std::cout << "no page migrations." << "\n\n";
			#endif

			return v;
		}
		#ifdef ENER_OUTPUT
		std::cout << "decided: let's migrate (up to " << MAX_PAGES_TO_MIGRATE << ") pages." << '\n';
		#endif

		// If we meet the criteria, we will migrate from the most loaded node to the least one
		const auto from = distance(rams.begin(), it_max);
		const auto to = distance(rams.begin(), it_min);

		#ifdef ENER_OUTPUT
		std::cout << "We will migrate pages from node " << from << " to node " << to << '\n';
		#endif

		// We search memory pages from "from" node to migrate
		for (auto const & t_it : page_ts) { // Looping over PIDs
			if (v.size() == MAX_PAGES_TO_MIGRATE)
				break;

			const auto pid = t_it.first;
			const auto table = t_it.second;

			for (auto const & p_it : table.page_node_map) { // Looping over pages
				if (v.size() == MAX_PAGES_TO_MIGRATE)
					break;

				const auto addr = p_it.first;
				const auto perf = p_it.second;

				// Not from the desired node, to the next!
				if (perf.current_node != from)
					continue;

				// We won't migrate pages with "high" locality
				if (perf.acs_per_node[from] > 1)
					continue;

				const migration_cell_t mc(addr, to, pid, false);
				v.push_back(mc);
			}
		}

		#ifdef ENER_OUTPUT
		std::cout << "\n\n";
		#endif

		return v;
	}
};

#endif
