/*
 * Defines the system structure: how many CPUs are, where is each cpu/thread...
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <sched.h>
#ifndef FAKE_DATA
#include <numa.h>
#endif

#include <algorithm>
#include <map>
#include <vector>
#include <set>
using namespace std;

typedef struct system_struct {
	static int NUM_OF_CPUS;
	static int NUM_OF_MEMORIES;
	static int CPUS_PER_MEMORY;

	// To know where each CPU is (in terms of memory node)
	static int* cpu_node_map; // cpu_node_map[cpu] = node
	static int** node_cpu_map; // cpu_node_map[node] = list(cpus)

	// To know where each TID is (in terms of CPUs)
	static map<pid_t, int> tid_cpu_map; // input: tid, output: cpu
	static vector<vector<pid_t>> cpu_tid_map; // input: cpu, output, list of tids

	static int** node_distances;

	// To know where each TID is (in terms of PIDs)
	static map<pid_t, pid_t> tid_pid_map; // input: tid, output: pid

	static vector<unsigned short> ordered_cpus; // Ordered by distance nodes. Might be useful for genetic strategy

	static int detect_system();
	static void clean();

	// Node-CPU methods
	static int get_cpu_memory_node(int cpu);
	static bool is_in_same_memory_node(int cpu1, int cpu2);
	static int get_random_cpu_in_node(int node);

	// CPU-thread methods
	static void add_tid(pid_t tid, int cpu);
	static int get_cpu_from_tid(pid_t tid);
	static vector<pid_t> get_tids_from_cpu(int cpu);
	static int set_tid_cpu(pid_t tid, int cpu, bool do_pin);
	static void remove_tid(pid_t tid, bool do_unpin);
	static bool is_cpu_free(int cpu);
	static int get_free_cpu_from_node(int node, set<int> nopes);

	// CPU-pin/free methods
	static int pin_thread_to_cpu(pid_t tid, int cpu);
	static int unpin_thread(pid_t tid);

	// Node distance methods
	static int get_node_distance(int node1, int node2);
	static void print_node_distance_matrix();

	// Other functions
	static void set_pid_to_tid(pid_t pid, pid_t tid);
	static pid_t get_pid_from_tid(pid_t tid);
} system_struct_t;

