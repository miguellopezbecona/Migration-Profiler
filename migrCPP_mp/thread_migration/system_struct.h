/*
 * Defines the system structure
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <vector>
using namespace std;

#define MAX_PACKAGES 16

#define SYS_IMPOSSIBLE_LATENCY -1
#define SYS_IMPOSSIBLY_LARGE_LATENCY 99999
#define SYS_CACHE_LINE_SIZE 64

#define SYS_ROOF_SAMPLES_CUTOFF 0

#define SYS_RESET_STEP 10
#define SYS_CUTOFF_STEP 40

extern int SYS_NUM_OF_CORES;
extern int SYS_NUM_OF_MEMORIES;

extern int SYS_CORES_PER_MEMORY;
extern int* cpu_node_map;
extern vector<int> node_cpu_map[MAX_PACKAGES];

int detect_system();
int get_cpu_memory_cell(int cpu);
bool is_in_same_memory_cell(int cpu1,int cpu2);
int get_random_core_in_cell(int cell);

