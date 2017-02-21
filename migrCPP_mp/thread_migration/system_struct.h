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

extern int SYS_NUM_OF_CORES;
extern int SYS_NUM_OF_MEMORIES;

extern int SYS_CORES_PER_MEMORY;
extern int* cpu_node_map;
extern vector<int> node_cpu_map[MAX_PACKAGES];

int detect_system();
int get_cpu_memory_cell(int cpu);
bool is_in_same_memory_cell(int cpu1,int cpu2);
int get_random_core_in_cell(int cell);

