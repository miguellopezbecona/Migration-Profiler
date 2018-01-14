#pragma once

#include <stdio.h>

#include <algorithm> // find
#include <map>
#include <vector>
using namespace std;

#include "gen_utils.h" // perf_data
#include "../../migration/migration_cell.h"
#include "../../migration/page_table.h"

typedef pid_t ind_type;

typedef struct individual {
	vector<ind_type> v; // Internal representation: list of TIDs where the index indicates the (ordered) CPU
	double fitness; // Currently defined as the mean latency of all the system

	individual();
	individual(map<pid_t, page_table_t> ts);
	individual(vector<pid_t> vec);

	double get_fitness() const;
	size_t size();
	void set(int index, ind_type value);
	ind_type get(int index);

	migration_cell_t mutate(int index); // Changes location directly
	void mutate(int idx1, int idx2); // Interchange

	individual cross(individual r, int idx1, int idx2); // Using order crossover
	individual get_copy();

	void print();
} individual_t;
