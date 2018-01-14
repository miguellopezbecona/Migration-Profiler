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

#include <map>
#include <vector>
using namespace std;

#define MAX_PACKAGES 8

typedef struct system_struct {
	static int NUM_OF_CPUS;
	static int NUM_OF_MEMORIES;
	static int CPUS_PER_MEMORY;
	static const int FREE_CPU = -1;

	// To know where each CPU is (in terms of memory node)
	static int* cpu_node_map; // cpu_node_map[cpu] = node
	static vector<int> node_cpu_map[MAX_PACKAGES]; // cpu_node_map[node] = list(cpus)

	// To know where each TID is (in terms of CPUs)
	static map<pid_t, int> tid_cpu_map; // input: tid, output: cpu
	static int* cpu_tid_map; // input: cpu, output, tid

	static int** node_distances;

	static vector<unsigned short> ordered_cpus; // Ordered by distance nodes. Might be useful for genetic strategy


	static int detect_system();

	// Node-CPU methods
	static int get_cpu_memory_node(int cpu);
	static bool is_in_same_memory_node(int cpu1, int cpu2);
	static int get_random_cpu_in_node(int node);

	// CPU-thread methods
	static int get_cpu_from_tid(pid_t tid);
	static int get_tid_from_cpu(int cpu);
	static int set_tid_cpu(pid_t tid, int cpu, bool do_pin);
	static void remove_tid(pid_t tid, bool do_unpin);
	static bool is_cpu_free(int cpu);

	// CPU-pin/free methods
	static int pin_thread_to_cpu(pid_t tid, int cpu);
	static int unpin_thread(pid_t tid);

	// Node distance methods
	static int get_node_distance(int node1, int node2);
	static void print_node_distance_matrix();
} system_struct_t;

