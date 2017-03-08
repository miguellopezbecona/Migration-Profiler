#include "genetic.h"

void genetic::do_genetic(individual r){	

	// First iteration. No other individuals so only mutation will be done
	if(p.v.empty()){
		mutation(&r);
		//p.add(r); // Final result is appended
		it = 1;
		best_sol = r;
		best_it = 0;
		return;
	}

	/** Tournament, cross, mutation, sort and replacement **/
	// Right now, all this will be skipped
/*
	individual chosen = tournament();
	printf("The following individual has won the tournament:\n");
	chosen.print();

	individual aux = cross(chosen, r);

	printf("Mutation process:\n");
	mutation(&aux);
	
	p.add(aux); // Final result is appended to population	
	p.customSort(); // Sorts by fitness, not necessary
	//printf("Population sorted by fitness. End of iteration.\n");
	p.print(); // Prints poblation content


	individual best = p.get_best_ind();

	// Updates best_sol if needed
	if(best.fitness() < best_sol.fitness()){
		best_sol = wot;
		best_it = it;
	}
*/
	it++;

	printf("\n");
}

// In this simplified version, we only select one individual (the one to be crossed with the current individual)
// so this can be implemented simply as picking the best (lowest fitness)
individual genetic::tournament(){
	return p.get_best_ind();
}
    
// Crosses tournament winner with current individual
individual genetic::cross(individual from_iter, individual chosen){
	size_t sz1 = from_iter.size();
	size_t sz2 = chosen.size();
	size_t sz = sz1;

	// We keep the minimum size to "avoid" (no) crossing issues
	if(sz2 < sz1)
		sz = sz2;

	// Cross probability, it will also be reused
	double prob = utils::obtain_double();
	printf("Cross: (rand num: %.2f) -> ", prob);
	
	if(prob > CROSS_PROB){
		printf("cross not done\n");
		return from_iter;
	} else {
		int ind1, ind2;
		prob = utils::obtain_double();
		ind1 = (int) round(prob*(sz-1));
		prob = utils::obtain_double();
		ind2 = (int) round(prob*(sz-1));

		// Two descendents can be obtained with order crossover
		individual r3 = chosen.cross(from_iter, ind1, ind2);
		individual r4 = from_iter.cross(chosen, ind1, ind2);
		printf("index cuts: %d %d\n", ind1, ind2);

		return r3; // Just one of them? Alternate each iteration?
	}

}
       
// Mutates current ind
void genetic::mutation(individual *r){
	// Just for current individual or for every one?
	
	//for(individual r : p.v){
		//printf("\tIND %d\n", i);
		size_t sz = r->size();
		for(size_t j=0;j<sz;j++){
			double prob, decimal;
			char action[20] = "does not mutate\0";

			// Obtains probability and mutation index
			prob = utils::obtain_double();
			if(prob > MUT_PROB){}
				//accion = "NO MUTA";
			else {
				decimal = utils::obtain_double();
				int ind = (int) round(decimal*(sz-1));
				sprintf(action, "interchanges with: %d", ind);

				// Key function
				r->mutate(j, ind);
			}

			printf("\tPosition: %lu (rand rum %.2f) %s\n", j, prob, action);
		}
	//}
}

void genetic::print_best_sol(){
	printf("\nBEST SOLUTION:\n");
	best_sol.print();
	printf("Fitness: %d\n", best_sol.fitness());
	printf("Obtained in iteration: %d\n", best_it);
}

vector<migration_cell_t> genetic_t::get_pages_to_migrate(page_table_t *page_t){
	vector<migration_cell_t> ret;

	// [TODO]: convert page_node_map and/or tid_node_map to a individual. There may be problems because those fields aren't "linear"
	individual ind;
	do_genetic(ind);

	// [TODO]: once the iteration is over, the differences regarding the passed individual should be calculated so we can know what to actually migrate

	return ret;
}

// Right now, for simplicity
vector<migration_cell_t> genetic_t::get_threads_to_migrate(page_table_t *page_t){
	return get_pages_to_migrate(page_t);
}

