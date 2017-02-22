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

extern int SYS_NUM_OF_CORES;
extern int SYS_NUM_OF_MEMORIES;
extern int SYS_CORES_PER_MEMORY;

// To know where each CPU is
extern int* cpu_node_map;
extern vector<int> node_cpu_map[MAX_PACKAGES];

// To know where each TID is
extern map<int, int> tid_core_map;
extern int* core_tid_map;

int detect_system();
int get_cpu_memory_cell(int cpu);
bool is_in_same_memory_cell(int cpu1, int cpu2);
int get_random_core_in_cell(int cell);
int get_tid_core(pid_t tid);
