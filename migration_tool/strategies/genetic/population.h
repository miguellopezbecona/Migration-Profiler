#pragma once

#include <vector>
#include <algorithm> // sort

#include "individual.h"

using namespace std;

const int MAX_SIZE = 5;

class population {    
	public:
	vector<individual> v;
	
	population();
	
	void add(individual ind);
	void set(int idx, individual indv);
	void update_fitness(double fitness); // A posteriori performance of a state
	individual get_best_ind();
	bool is_first_iteration();

	void print();
};
