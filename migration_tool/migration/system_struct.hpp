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

class system_struct_t {
public:
	static int NUM_OF_CPUS;
	static int NUM_OF_MEMORIES;
	static int CPUS_PER_MEMORY;

	// To know where each CPU is (in terms of memory node)
	static std::vector<int> cpu_node_map; // cpu_node_map[cpu] = node
	static std::vector<std::vector<int>> node_cpu_map; // cpu_node_map[node] = list(cpus)

	// To know where each TID is (in terms of CPUs)
	static std::map<pid_t, int> tid_cpu_map; // input: tid, output: cpu
	static std::vector<std::vector<pid_t>> cpu_tid_map; // input: cpu, output, list of tids

	// static int** node_distances;
	static std::vector<std::vector<int>> node_distances;

	// To know where each TID is (in terms of PIDs)
	static std::map<pid_t, pid_t> tid_pid_map; // input: tid, output: pid

	static std::vector<unsigned short> ordered_cpus; // Ordered by distance nodes. Might be useful for genetic strategy

private:
	/*** Auxiliar functions ***/
	static void set_affinity_error (const pid_t tid) {
		switch(errno){
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
	static void read_line_from_file (const int node, std::vector<int> & array) {
		char filename[64] = "\0";
		sprintf(filename, "/sys/devices/system/node/node%d/distance", node);
		std::ifstream file(filename);

		if (!file.is_open())
			return;

		// for(int i=0;fscanf(file, "%d", &array[i]) == 1; i++);
		while (!file.eof()) {
			int element;
			file >> element;
			array.push_back(element);
		}
		array.shrink_to_fit();

		file.close();
	}

	static bool are_all_nodes_processed (const std::vector<bool> processed) {
		for (int i = 0; i < system_struct_t::NUM_OF_MEMORIES; i++) {
			if (!processed[i])
				return false;
		}
		return true;
	}

public:
	static int detect_system () {
		char filename[BUFSIZ];
		int package;

		NUM_OF_CPUS = sysconf(_SC_NPROCESSORS_ONLN);
		NUM_OF_MEMORIES = numa_num_configured_nodes();
		CPUS_PER_MEMORY = NUM_OF_CPUS / NUM_OF_MEMORIES;

		node_cpu_map = std::vector<std::vector<int>>(NUM_OF_MEMORIES);
		for (int i = 0; i < NUM_OF_MEMORIES; i++) {
			std::vector<int> v(CPUS_PER_MEMORY, 0);
			node_cpu_map[i] = v;
		}

		int counters[CPUS_PER_MEMORY]; // For keeping indexes to build node_cpu_map
		memset(counters, 0, sizeof(counters));

		cpu_node_map.reserve(NUM_OF_CPUS);
		cpu_tid_map.resize(NUM_OF_CPUS);

		// For each CPU, reads topology file to get package (node) id
		for (int i = 0; i < NUM_OF_CPUS; i++) {
			sprintf(filename, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", i);
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
		node_distances = std::vector<std::vector<int>>(NUM_OF_MEMORIES);
		for (int i = 0; i < NUM_OF_MEMORIES; i++) {
			std::vector<int> v(NUM_OF_MEMORIES);
			node_distances[i] = v;
			read_line_from_file(i, node_distances[i]);
		}


		std::cout << "Detected system: " << NUM_OF_CPUS << " total CPUs, " << NUM_OF_MEMORIES << " memory nodes, " << CPUS_PER_MEMORY << " CPUs per node." << '\n';

		//print_node_distance_matrix();

		// We will calculate ordered_cpus (does not take into account hyperthreading, so a core won't be next to its HT partner)
		ordered_cpus.reserve(NUM_OF_CPUS);

		std::vector<bool> processed(NUM_OF_MEMORIES, false);
		// processed.reserve(NUM_OF_MEMORIES);
		// memset(processed, false, NUM_OF_MEMORIES*sizeof(bool));

		// We begin with node 0
		int ref_node = 0;
		ordered_cpus.insert(ordered_cpus.end(), node_cpu_map[0].begin(), node_cpu_map[0].begin() + CPUS_PER_MEMORY);
		processed[0] = true;
		while (!are_all_nodes_processed(processed)) {

			// We get the nearest not processed node to the current one
			int min_index = -1;
			int min_distance = 1e3;
			for (int i = 0; i < NUM_OF_MEMORIES; i++) {
				if (i == ref_node || processed[i])
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
		// free(cpu_node_map);
		// for (int i = 0; i < NUM_OF_MEMORIES; i++) {
		// 	free(node_cpu_map[i]);
		// 	free(node_distances[i]);
		// }
		// free(node_distances);
		// free(node_cpu_map);
		// for (auto & v : node_cpu_map) {
		// 	v->clear();
		// }
		// TODO: necessary
		node_cpu_map.clear();
		tid_cpu_map.clear();
		for(int i = 0; i < NUM_OF_CPUS; i++) {
			cpu_tid_map[i].clear();
		}
		cpu_tid_map.clear();
		tid_pid_map.clear();
		ordered_cpus.clear();
	}

	// Node-CPU methods
	static inline int get_cpu_memory_node (const int cpu) {
		return cpu_node_map[cpu];
	}

	static inline bool is_in_same_memory_node(int cpu1, int cpu2) {
		return cpu_node_map[cpu1] == cpu_node_map[cpu2];
	}

	static inline int get_random_cpu_in_node (const int node) {
		int position = rand() % CPUS_PER_MEMORY;
		return node_cpu_map[node][position];
	}

	// CPU-thread methods
	static inline void add_tid (const pid_t tid, const int cpu) {
		if (tid_cpu_map.count(tid)) // We don't continue if the TID already exists in map
			return;

		if (is_cpu_free(cpu)) { // The thread is also pinned to the CPU if it is free
			set_tid_cpu(tid, cpu, true);
			return;
		}

		// If not, we search a free CPU on its same node
		int node = cpu_node_map[cpu];
		for (int i = 0; i < CPUS_PER_MEMORY; i++) {
			int other_cpu = node_cpu_map[node][i];
			if (is_cpu_free(other_cpu)) {
				set_tid_cpu(tid, other_cpu, true);
				return;
			}
		}

		// If we are here, we didn't found a free CPU in the memory node of the CPU where the sample was gotten
		// We will pin it to the initial CPU anyway
		set_tid_cpu(tid, cpu, true);
	}

	static int get_cpu_from_tid (const pid_t tid) {
		return tid_cpu_map[tid];
	}

	static inline std::vector<pid_t> get_tids_from_cpu (const int cpu) {
		return cpu_tid_map[cpu];
	}

	static int set_tid_cpu (const pid_t tid, const int cpu, const bool do_pin) {
		// We drop the previous CPU-TID assignation, if there was one
		if (tid_cpu_map.count(tid)) { // contains
			int old_cpu = tid_cpu_map[tid];

			// Drops TID from the list
			std::vector<pid_t> & l = cpu_tid_map[old_cpu];
			l.erase(remove(l.begin(), l.end(), tid), l.end());
		}

		tid_cpu_map[tid] = cpu;

		if (!do_pin)
			return 0;

		cpu_tid_map[cpu].push_back(tid);
		return pin_thread_to_cpu(tid, cpu);
	}

	static void remove_tid (const pid_t tid, const bool do_unpin) {
		tid_pid_map.erase(tid);

		int cpu = tid_cpu_map[tid];
		tid_cpu_map.erase(tid);

		std::vector<pid_t> & l = cpu_tid_map[cpu];
		l.erase(remove(l.begin(), l.end(), tid), l.end());

		// Already finished threads don't need unpin
		if (do_unpin)
			unpin_thread(tid);
	}

	static inline bool is_cpu_free (const int cpu) {
		return cpu_tid_map[cpu].empty();
	}

	static inline int get_free_cpu_from_node(const int node, const std::set<int> nopes) {
		for (int c = 0; c < CPUS_PER_MEMORY; c++) {
			int cpu = node_cpu_map[node][c];
			if (is_cpu_free(cpu) && !nopes.count(cpu))
				return cpu;
		}

		// If no free/feasible CPUs, picks any random one
		int index = rand() % CPUS_PER_MEMORY;
		return node_cpu_map[node][index];
	}

	// CPU-pin/free methods
	static inline int pin_thread_to_cpu (const pid_t tid, const int cpu) {
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

	static inline int unpin_thread (const pid_t tid) {
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
	static inline int get_node_distance (const int node1, const int node2) {
		return node_distances[node1][node2];
	}

	static void print_node_distance_matrix () {
		for (int i = 0; i < NUM_OF_MEMORIES; i++) {
			for (int j = 0; j < NUM_OF_MEMORIES; j++) {
				std::cout << node_distances[i][j] << ' ';
			}
			std::cout << '\n';
		}
	}

	// Other functions
	static inline void set_pid_to_tid (const pid_t pid, const pid_t tid) {
		tid_pid_map[tid] = pid;
	}

	static inline pid_t get_pid_from_tid (const pid_t tid) {
		if (tid_pid_map.count(tid) > 0)
			return tid_pid_map[tid];
		else
			return -1;
	}
};

// Declaration of static variables
int system_struct_t::NUM_OF_CPUS;
int system_struct_t::NUM_OF_MEMORIES;
int system_struct_t::CPUS_PER_MEMORY;
// To know where each CPU is (in terms of memory node)
std::vector<int> system_struct_t::cpu_node_map; // cpu_node_map[cpu] = node
std::vector<std::vector<int>> system_struct_t::node_cpu_map; // cpu_node_map[node] = list(cpus)
// To know where each TID is (in terms of CPUs)
std::map<pid_t, int> system_struct_t::tid_cpu_map; // input: tid, output: cpu
std::vector<std::vector<pid_t>> system_struct_t::cpu_tid_map; // input: cpu, output, list of tids
std::vector<std::vector<int>> system_struct_t::node_distances;
// To know where each TID is (in terms of PIDs)
std::map<pid_t, pid_t> system_struct_t::tid_pid_map; // input: tid, output: pid
std::vector<unsigned short> system_struct_t::ordered_cpus; // Ordered by distance nodes. Might be useful for genetic strategy

#endif
