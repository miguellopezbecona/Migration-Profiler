/*
 * Defines the system structure: how many CPUs are, where is each cpu/thread...
 */
#pragma once
#ifndef __SYSTEM_STRUCT__
#define __SYSTEM_STRUCT__

#include <iostream>
#include <fstream>

#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <errno.h>
#include <sched.h>
#include <numa.h>

#include <algorithm>
#include <map>
#include <vector>
#include <set>

#include "types_definition.hpp"

class system_struct_t {
public:
	static size_t NUM_OF_CPUS;
	static size_t NUM_OF_MEMORIES;
	static size_t CPUS_PER_MEMORY;

	// To know where each CPU is (in terms of memory node)
	static std::vector<node_t> cpu_node_map; // cpu_node_map[cpu] = node
	static std::vector<std::vector<cpu_t>> node_cpu_map; // cpu_node_map[node] = list(cpus)

	// To know where each TID is (in terms of CPUs)
	static std::map<tid_t, cpu_t> tid_cpu_map; // input: tid, output: cpu
	static std::vector<std::vector<tid_t>> cpu_tid_map; // input: cpu, output, list of tids

	// static int** node_distances;
	static std::vector<std::vector<size_t>> node_distances;

	// To know where each TID is (in terms of PIDs)
	static std::map<tid_t, pid_t> tid_pid_map; // input: tid, output: pid

	static std::vector<cpu_t> ordered_cpus; // Ordered by distance nodes. Might be useful for genetic strategy

private:
	/*** Auxiliar functions ***/
	static void set_affinity_error (const tid_t tid) {
		switch (errno) {
			case EFAULT:
				std::cerr << "Error setting affinity: A supplied memory address was invalid." << '\n';
				break;
			case EINVAL:
				std::cerr << "Error setting affinity: The affinity bitmask mask contains no processors that are physically on the system, or cpusetsize is smaller than the size of the affinity mask used by the kernel." << '\n';
				break;
			case EPERM:
				std::cerr << "Error setting affinity: The calling process does not have appropriate privileges for TID " << tid << "," << '\n';
				break;
			case ESRCH: // When this happens, it's practically unavoidable
				//printf("Error setting affinity: The process whose ID is %d could not be found\n", tid);
				break;
		}
	}

	// Each line will result in a row in the distance matrix
	template<class T>
	static void read_line_from_file (const T node, std::vector<T> & array) {
		char filename[64] = "\0";
		sprintf(filename, "/sys/devices/system/node/node%lu/distance", node);
		std::ifstream file(filename);

		if (!file.is_open())
			return;

		for (size_t i = 0; !file.eof() && i < array.size(); i++) {
			T element;
			file >> element;
			if (file.eof())
				break;
			array[i] = element;
		}
		array.shrink_to_fit();

		file.close();
	}

	static bool are_all_nodes_processed (const std::vector<bool> & processed) {
		for (size_t i = 0; i < system_struct_t::NUM_OF_MEMORIES; i++) {
			if (!processed[i])
				return false;
		}
		return true;
	}

public:
	static int detect_system () {
		char filename[BUFSIZ]; // BUFSIZ is a system macro.
		int package;

		NUM_OF_CPUS = sysconf(_SC_NPROCESSORS_ONLN);
		NUM_OF_MEMORIES = numa_num_configured_nodes();
		CPUS_PER_MEMORY = NUM_OF_CPUS / NUM_OF_MEMORIES;

		node_cpu_map = std::vector<std::vector<cpu_t>>(NUM_OF_MEMORIES);
		for (size_t i = 0; i < NUM_OF_MEMORIES; i++) {
			node_cpu_map[i].reserve(CPUS_PER_MEMORY);
		}

		std::vector<size_t> counters(CPUS_PER_MEMORY, 0); // For keeping indexes to build node_cpu_map

		cpu_node_map.reserve(NUM_OF_CPUS);
		cpu_tid_map.resize(NUM_OF_CPUS);

		// For each CPU, reads topology file to get package (node) id
		for (size_t i = 0; i < NUM_OF_CPUS; i++) {
			sprintf(filename, "/sys/devices/system/cpu/cpu%lu/topology/physical_package_id", i);
			std::ifstream file(filename);
			if (!file.is_open()) {
				break;
			}
			if (!(file >> package)) {
				return -1;
			}
			file.close();

			// Saves data to structures
			int index = counters[package];
			node_cpu_map[package][index] = i;
			cpu_node_map[i] = package;

			counters[package]++;
		}

		// Initializes and builds node distance matrix
		node_distances = std::vector<std::vector<size_t>>(NUM_OF_MEMORIES);
		for (size_t i = 0; i < NUM_OF_MEMORIES; i++) {
			node_distances[i] = std::vector<size_t>(NUM_OF_MEMORIES);
			read_line_from_file(i, node_distances[i]);
		}

		std::cout << "Detected system: " << NUM_OF_CPUS << " total CPUs, " << NUM_OF_MEMORIES << " memory nodes, " << CPUS_PER_MEMORY << " CPUs per node." << '\n';

		//print_node_distance_matrix();

		// We will calculate ordered_cpus (does not take into account hyperthreading, so a core won't be next to its HT partner)
		ordered_cpus.reserve(NUM_OF_CPUS);

		std::vector<bool> processed(NUM_OF_MEMORIES, false);

		// We begin with node 0
		node_t ref_node = 0;
		ordered_cpus.insert(ordered_cpus.end(), node_cpu_map[0].begin(), node_cpu_map[0].begin() + CPUS_PER_MEMORY);
		processed[0] = true;
		while (!are_all_nodes_processed(processed)) {
			// We get the nearest not processed node to the current one
			size_t min_index;
			size_t min_distance = 1e3;
			for (size_t i = 0; i < NUM_OF_MEMORIES; i++) {
				if (i == static_cast<size_t>(ref_node) || processed[i])
					continue;

				if (node_distances[ref_node][i] < min_distance) {
					min_distance = node_distances[ref_node][i];
					min_index = i;
				}
			}

			// We got the new mininum: mark as processed and concat its CPUs
			processed[min_index] = true;
			ordered_cpus.insert(ordered_cpus.end(), node_cpu_map[min_index].begin(), node_cpu_map[min_index].end() + CPUS_PER_MEMORY);

			ref_node = min_index;
		}

		// Now we should have the CPUs ordered as we want
	/*
		for(unsigned short c : ordered_cpus)
			printf("%d ", c);
		printf("\n");
	*/

		return 0;
	}

	static void clean () {
		cpu_node_map.clear();
		for(int i=0;i<NUM_OF_MEMORIES;i++){
			node_cpu_map[i].clear();
			node_distances[i].clear();
		}
		node_distances.clear();
		node_cpu_map.clear();
		tid_cpu_map.clear();

		for (size_t i = 0; i < NUM_OF_CPUS; i++)
			cpu_tid_map[i].clear();

		cpu_tid_map.clear();
		ordered_cpus.clear();
		tid_pid_map.clear();
	}

	// Node-CPU methods
	static inline node_t get_cpu_memory_node (const cpu_t cpu) {
		return cpu_node_map[cpu];
	}

	static inline bool is_in_same_memory_node(const cpu_t cpu1, const cpu_t cpu2) {
		return cpu_node_map[cpu1] == cpu_node_map[cpu2];
	}

	static inline cpu_t get_random_cpu_in_node (const node_t node) {
		const auto position = rand() % CPUS_PER_MEMORY;
		return node_cpu_map[node][position];
	}

	// CPU-thread methods
	static inline void add_tid (const tid_t tid, const cpu_t cpu) {
		if (tid_cpu_map.count(tid)) // We don't continue if the TID already exists in map
			return;

		if (is_cpu_free(cpu)) { // The thread is also pinned to the CPU if it is free
			set_tid_cpu(tid, cpu, true);
			return;
		}

		// If not, we search a free CPU on its same node
		const auto node = cpu_node_map[cpu];
		for (size_t i = 0; i < CPUS_PER_MEMORY; i++) {
			const auto other_cpu = node_cpu_map[node][i];
			if (is_cpu_free(other_cpu)) {
				set_tid_cpu(tid, other_cpu, true);
				return;
			}
		}

		// If we are here, we didn't found a free CPU in the memory node of the CPU where the sample was gotten
		// We will pin it to the initial CPU anyway
		set_tid_cpu(tid, cpu, true);
	}

	static cpu_t get_cpu_from_tid (const tid_t tid) {
		return tid_cpu_map[tid];
	}

	static inline std::vector<tid_t> get_tids_from_cpu (const cpu_t cpu) {
		return cpu_tid_map[cpu];
	}

	static int set_tid_cpu (const tid_t tid, const cpu_t cpu, const bool do_pin) {
		// We drop the previous CPU-TID assignation, if there was one
		if (tid_cpu_map.count(tid)) { // contains
			const auto old_cpu = tid_cpu_map[tid];

			// Drops TID from the list
			auto & l = cpu_tid_map[old_cpu];
			l.erase(std::remove(l.begin(), l.end(), tid), l.end());
		}

		tid_cpu_map[tid] = cpu;

		if (!do_pin)
			return 0;

		cpu_tid_map[cpu].push_back(tid);
		return pin_thread_to_cpu(tid, cpu);
	}

	static void remove_tid (const tid_t tid, const bool do_unpin) {
		tid_pid_map.erase(tid);

		const auto cpu = tid_cpu_map[tid];
		tid_cpu_map.erase(tid);

		auto & l = cpu_tid_map[cpu];
		l.erase(remove(l.begin(), l.end(), tid), l.end());

		// Already finished threads don't need unpin
		if (do_unpin)
			unpin_thread(tid);
	}

	static inline bool is_cpu_free (const cpu_t cpu) {
		return cpu_tid_map[cpu].empty();
	}

	static inline cpu_t get_free_cpu_from_node(const node_t node, const std::set<int> & nopes) {
		for (size_t c = 0; c < CPUS_PER_MEMORY; c++) {
			const auto cpu = node_cpu_map[node][c];
			if (is_cpu_free(cpu) && !nopes.count(cpu))
				return cpu;
		}

		// If no free/feasible CPUs, picks any random one
		size_t index = rand() % CPUS_PER_MEMORY;
		return node_cpu_map[node][index];
	}

	// CPU-pin/free methods
	static inline int pin_thread_to_cpu (const tid_t tid, const cpu_t cpu) {
		cpu_set_t affinity;
		sched_getaffinity(tid, sizeof(cpu_set_t), &affinity);
		int ret = 0;

		CPU_ZERO(&affinity);
		CPU_SET(cpu, &affinity);

		if ( (ret = sched_setaffinity(tid, sizeof(cpu_set_t), &affinity)) ) {
			set_affinity_error(tid);
			return errno;
		}

		return ret;
	}

	static inline int unpin_thread (const tid_t tid) {
		cpu_set_t affinity;
		sched_getaffinity(0, sizeof(cpu_set_t), &affinity); // Gets profiler's affinity (all CPUs)
		int ret = 0;

		// Gives profiler's affinity to freed thread
		if ( (ret = sched_setaffinity(tid, sizeof(cpu_set_t), &affinity)) ) {
			set_affinity_error(tid);
			return errno;
		}

		return ret;
	}

	// Node distance methods
	static inline size_t get_node_distance (const node_t node1, const node_t node2) {
		return node_distances[node1][node2];
	}

	static void print_node_distance_matrix () {
		for (size_t i = 0; i < NUM_OF_MEMORIES; i++) {
			for (size_t j = 0; j < NUM_OF_MEMORIES; j++) {
				std::cout << node_distances[i][j] << ' ';
			}
			std::cout << '\n';
		}
	}

	// Other functions
	static inline void set_pid_to_tid (const pid_t pid, const tid_t tid) {
		tid_pid_map[tid] = pid;
	}

	static inline pid_t get_pid_from_tid (const tid_t tid) {
		if (tid_pid_map.count(tid) > 0)
			return tid_pid_map[tid];
		else
			return -1;
	}
};

// Declaration of static variables
size_t system_struct_t::NUM_OF_CPUS;
size_t system_struct_t::NUM_OF_MEMORIES;
size_t system_struct_t::CPUS_PER_MEMORY;
// To know where each CPU is (in terms of memory node)
std::vector<node_t> system_struct_t::cpu_node_map; // cpu_node_map[cpu] = node
std::vector<std::vector<cpu_t>> system_struct_t::node_cpu_map; // cpu_node_map[node] = list(cpus)
// To know where each TID is (in terms of CPUs)
std::map<tid_t, cpu_t> system_struct_t::tid_cpu_map; // input: tid, output: cpu
std::vector<std::vector<tid_t>> system_struct_t::cpu_tid_map; // input: cpu, output, list of tids
std::vector<std::vector<size_t>> system_struct_t::node_distances;
// To know where each TID is (in terms of PIDs)
std::map<tid_t, pid_t> system_struct_t::tid_pid_map; // input: tid, output: pid
std::vector<cpu_t> system_struct_t::ordered_cpus; // Ordered by distance nodes. Might be useful for genetic strategy

#endif
