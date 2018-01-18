#pragma once

#include "individual.h"

const int MAX_SIZE = 5;

class population {    
	public:
	vector<individual> v;
	
	population();
	
	void add(individual ind);
	void set(int idx, individual indv);
	void update_fitness(double fitness); // A posteriori performance update the latest-1_th state
	individual get_best_ind();
	bool is_first_iteration();

	void print();
};
