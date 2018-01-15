#pragma once

#include <stdio.h>

#include <algorithm> // find, count
#include <set>
#include <vector>
using namespace std;

#include "gen_utils.h"
#include "../../migration/migration_cell.h"
#include "../../migration/page_table.h" // perf_data

typedef pid_t ind_type; // TID

typedef struct individual {
	vector<ind_type> v; // Internal representation: list of TIDs where the index indicates the (ordered) CPU
	double fitness; // Currently defined as the mean latency of all the system. The lower the better

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

	// For easing readibility
	ind_type operator[](int i) const;
	ind_type & operator[](int i);
} individual_t;
