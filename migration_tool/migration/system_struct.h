/*
 * Defines the system structure: how many cores are, where is each cpu/thread...
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <map>
#include <vector>
using namespace std;

#define MAX_PACKAGES 8

typedef struct system_struct {
	static int NUM_OF_CORES;
	static int NUM_OF_MEMORIES;
	static int CORES_PER_MEMORY;

	// To know where each CPU is (in terms of memory node)
	static int* cpu_node_map; // cpu_node_map[cpu] = node
	static vector<int> node_cpu_map[MAX_PACKAGES]; // cpu_node_map[node] = list(cpus)

	// To know where each TID is (in terms or cores)
	static map<pid_t, int> tid_core_map;
	static int* core_tid_map;

	static int detect_system();

	static int get_cpu_memory_cell(int cpu);
	static bool is_in_same_memory_cell(int cpu1, int cpu2);
	static int get_random_core_in_cell(int cell);
	static int get_tid_core(pid_t tid);
	static void set_tid_core(pid_t tid, int core);
	static void remove_tid(pid_t tid);
} system_struct_t;

