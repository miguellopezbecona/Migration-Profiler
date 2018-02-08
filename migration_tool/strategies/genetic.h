#pragma once

#include "strategy.h"

#include "genetic/population.h"
#include "genetic/gen_utils.h"
#include "../migration/migration_cell.h"

const double CROSS_PROB = 0.5;
const double MUT_PROB = 0.1;

typedef struct genetic : strategy {
	population p;
	int it;
	int best_it;
	individual best_sol;

	genetic();

	vector<migration_cell_t> do_genetic(individual ind);
	individual tournament();
	individual cross(individual from_iter, individual chosen, vector<migration_cell_t> *v);
	void mutation(individual *ind, vector<migration_cell_t> *v);
	void print_best_sol();

	// The following should not be written because it should inherit it from strategy
	vector<migration_cell_t> get_pages_to_migrate(map<pid_t, page_table_t> *page_ts);
	vector<migration_cell_t> get_threads_to_migrate(map<pid_t, page_table_t> *page_ts);
} genetic_t;

