#pragma once

#include <stdio.h>

#include <algorithm> // find
#include <map>
#include <vector>
using namespace std;

#include "../../migration/page_table.h" // perf_data
#include "../../migration/migration_cell.h"

class individual {
	private:
	// Migration cell instead of just numbers (locations) so we can keep which memory page or TID is associated to that location
	//vector<migration_cell_t> v;
    
	public:
	vector<migration_cell_t> v;

	individual();
	individual(map<long int, perf_data_t> m);
	individual(vector<migration_cell_t> vec);

	int fitness() const;

	size_t size();
	void set(int index, int value);
	int get(int index);

	void mutate(int idx1, int idx2);
	individual cross(individual r, int idx1, int idx2); // Using order crossover
	individual get_copy();

	void print();

	bool operator < (const individual &otro) const; 
};
