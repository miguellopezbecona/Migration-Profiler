#pragma once

#include "gen_utils.h" // MAX/MIN macro
#include "../../migration/page_table.h" // It uses it in one constructor

// This would fit better in genetic.h, but it raised double link problems
#define GENETIC_OUTPUT

typedef pid_t gene; // TID

#ifdef MAXIMIZATION
const double NO_FITNESS = -1.0;
#elif defined(MINIMIZATION)
const double NO_FITNESS = 1e6;
#endif

typedef struct individual {
	vector<gene> v; // Internal representation: list of TIDs where the index indicates the (ordered) CPU
	double fitness; // Currently defined as the mean latency of all the system. The lower the better

	individual();
	individual(map<pid_t, page_table_t> ts);
	individual(vector<gene> vec);

	double get_fitness() const;
	size_t size();
	void set(int index, gene value);
	bool is_too_old();

	migration_cell_t mutate(int index); // Changes location directly
	void mutate(int idx1, int idx2); // Interchange

	individual cross(individual r, int idx1, int idx2); // Using order crossover
	individual get_copy();

	void print() const;

	// For easing readibility
	gene operator[](int i) const;
	gene & operator[](int i);
} individual_t;
