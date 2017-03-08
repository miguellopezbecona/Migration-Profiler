#pragma once

#include <stdio.h>

#include <algorithm> // find
#include <vector>
using namespace std;

class individual {
	private:
	vector<int> v;
    
	public:

	individual();
	individual(vector<int> vec);

	int fitness() const;

	size_t size();
	void add(int value);
	void set(int index, int value);
	int get(int index);
	bool contains(int city);

	void mutate(int idx1, int idx2);
	individual cross(individual r, int idx1, int idx2); // Using order crossover
	individual get_copy();

	void print();

	bool operator < (const individual &otro) const; 
};
