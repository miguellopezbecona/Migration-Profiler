#pragma once

#include <stdio.h>

#include <algorithm> // find
#include <map>
#include <vector>
using namespace std;

#include "gen_utils.h" // perf_data
#include "../../migration/migration_cell.h"
#include "../../migration/page_table.h"

class individual {
	private:
	// Migration cell instead of just numbers (locations) so we can keep which PID and memory page/TID is associated to that location
	//vector<migration_cell_t> v;
    
	public:
	vector<migration_cell_t> v;

	individual();
	individual(map<pid_t, page_table_t> ts);
	individual(vector<migration_cell_t> vec);

	int fitness() const;

	size_t size();
	void set(int index, int value);
	int get(int index);

	migration_cell_t mutate(int index); // Changes location directly
	void mutate(int idx1, int idx2); // Interchange

	individual cross(individual r, int idx1, int idx2); // Using order crossover
	individual get_copy();

	void print();

	bool operator < (const individual &otro) const; 
};
