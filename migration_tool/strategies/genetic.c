#include "genetic.h"

vector<migration_cell_t> genetic::do_genetic(individual r){
	vector<migration_cell_t> v;

	// First iteration. No other individuals so only mutation will be done
	if(p.v.empty()){
		mutation(&r, &v);
		//p.add(r); // Final result is appended to historic population
		it = 1;
		best_sol = r;
		best_it = 0;
		return v;
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

	#ifdef GENETIC_OUTPUT
	printf("\n");
	#endif

	return v;
}

// In this simplified version, we only select one individual (the one to be crossed with the current individual)
// so this can be implemented simply as picking the best (lowest fitness)
individual genetic::tournament(){
	return p.get_best_ind();
}
    
// Crosses tournament winner with current individual. Should be adapted to create migration cells as well
individual genetic::cross(individual from_iter, individual chosen){
	size_t sz1 = from_iter.size();
	size_t sz2 = chosen.size();
	size_t sz = sz1;

	// We keep the minimum size to "avoid" (no) crossing issues
	if(sz2 < sz1)
		sz = sz2;

	// Cross probability
	double prob = gen_utils::get_rand_double();

	#ifdef GENETIC_OUTPUT
	printf("Cross: (rand num: %.2f) -> ", prob);
	#endif
	
	if(prob > CROSS_PROB){
		#ifdef GENETIC_OUTPUT
		printf("cross not done\n");
		#endif
		return from_iter;
	} else {
		int ind1 = gen_utils::get_rand_int(sz, -1);
		int ind2 = gen_utils::get_rand_int(sz, ind1);

		// Two descendents can be obtained with order crossover
		individual r3 = chosen.cross(from_iter, ind1, ind2);
		individual r4 = from_iter.cross(chosen, ind1, ind2);

		#ifdef GENETIC_OUTPUT
		printf("index cuts: %d %d\n", ind1, ind2);
		#endif

		return r3; // Just one of them? Alternate each iteration?
	}

}
       
// Mutates current individual
void genetic::mutation(individual *r, vector<migration_cell_t> *v){
	size_t sz = r->size();

	for(size_t pos1=0;pos1<sz;pos1++){

		// Obtains probability
		double prob = gen_utils::get_rand_double();

		#ifdef GENETIC_OUTPUT
		printf("\tposition: %lu (rand rum %.2f) -> ", pos1, prob);
		#endif

		if(prob > MUT_PROB){
			#ifdef GENETIC_OUTPUT
			printf("mutation not done\n");
			#endif
		} else {

			// Mutation by changing the location directly
			migration_cell_t mc = r->mutate(pos1);
			v->push_back(mc);

			#ifdef GENETIC_OUTPUT
			printf("mutated to %d\n", mc.dest);
			#endif

			// Mutation by interchange
/*
			int pos2 = gen_utils::get_rand_int(sz, pos1);

			// Generates two migration cells: r.v[pos1] goes to r.v[pos2] location and viceversa
			migration_cell_t mc1(r->v[pos1].elem, r->v[pos2].dest);
			migration_cell_t mc2(r->v[pos2].elem, r->v[pos1].dest);
			v->push_back(mc1);
			v->push_back(mc2);

			// Key function
			r->mutate(pos1, pos2);

			#ifdef GENETIC_OUTPUT
			printf("interchanges with position %d\n", pos2);
			#endif
*/

		}


	}
}

void genetic::print_best_sol(){
	printf("\nBEST SOLUTION:\n");
	best_sol.print();
	printf("Fitness: %d\n", best_sol.fitness());
	printf("Obtained in iteration: %d\n", best_it);
}

vector<migration_cell_t> genetic_t::get_pages_to_migrate(map<pid_t, page_table_t> *page_ts){
	// Converts both node_maps (page_node_map and tid_node_map) to a individual (list of cells). This may imply a slowdown
	individual ind(*page_ts);
	return do_genetic(ind);
}

// Right now, for simplicity:
vector<migration_cell_t> genetic_t::get_threads_to_migrate(map<pid_t, page_table_t> *page_ts){
	return get_pages_to_migrate(page_ts);
}

