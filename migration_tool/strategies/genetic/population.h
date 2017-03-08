#pragma once

#include <vector>
#include <algorithm> // sort

#include "individual.h"

using namespace std;

const int MAX_SIZE = 5;

class population {
	private:
	//vector<individual> v;
    
	public:
	vector<individual> v;
	
	population();
	
	void add(individual r);
	void set(int index, individual r);
	void customSort();
	individual get_best_ind();

	void print();
};
