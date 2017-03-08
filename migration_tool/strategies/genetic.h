#pragma

#include <math.h>

#include "strategy.h"

#include "genetic/population.h"
#include "genetic/individual.h"
#include "genetic/utils.h"

using namespace std;

const double CROSS_PROB = 0.9;
const double MUT_PROB = 0.08;

typedef struct genetic : strategy {
	population p;
	int it;
	int best_it;
	individual best_sol;

	genetic(){}

	void do_genetic(individual r);
	individual tournament();
	individual cross(individual from_iter, individual chosen);
	void mutation(individual *r);
	void print_best_sol();

	// The following should not be written because it should inherit it from strategy
	vector<migration_cell_t> get_pages_to_migrate(page_table_t *page_t);
	vector<migration_cell_t> get_threads_to_migrate(page_table_t *page_t);
} genetic_t;

