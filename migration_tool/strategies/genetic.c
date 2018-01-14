#include "genetic.h"

vector<migration_cell_t> genetic::do_genetic(individual r){
	vector<migration_cell_t> v;

	// First iteration. No other individuals so only mutation will be done
	if(p.v.empty()){
		mutation(&r, &v);
		p.add(r); // Final result is appended to historic population
		it = 1;
		best_sol = r;
		best_it = 1;
		return v;
	}

	/** Tournament, cross, mutation, sort and replacement **/
	// Right now, all this will be skipped
	individual chosen = tournament();

	#ifdef GENETIC_OUTPUT
	printf("The following individual has won the tournament:\n");
	chosen.print();
	#endif

	individual aux = cross(chosen, r);

	#ifdef GENETIC_OUTPUT
	printf("Mutation process:\n");
	#endif
	mutation(&aux, &v);
	
	p.add(aux); // Final result is appended to population

	#ifdef GENETIC_OUTPUT
	printf("End of iteration.\n");
	p.print(); // Prints poblation content
	#endif

	individual best = p.get_best_ind();

	// Updates best_sol if needed
	if(best.get_fitness() < best_sol.get_fitness()){
		best_sol = best;
		best_it = it;
	}
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
	size_t sz = from_iter.size();

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

		prob = gen_utils::get_rand_double();

		if(prob > 0.5){
			#ifdef GENETIC_OUTPUT
			printf("We kept first child.\n");
			#endif
			return r3;
		} else {
			#ifdef GENETIC_OUTPUT
			printf("We kept second child.\n");
			#endif
			return r4;
		}
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
			// Mutation by interchange
			int pos2 = gen_utils::get_rand_int(sz, pos1);

			// Generates two migration cells: r.v[pos1] goes to r.v[pos2] location and viceversa
			int dest1 = system_struct_t::ordered_cpus[pos2]; // CPU associated to pos2
			int dest2 = system_struct_t::ordered_cpus[pos1];

			migration_cell_t mc1(r->v[pos1], dest1);
			migration_cell_t mc2(r->v[pos2], dest2);
			v->push_back(mc1);
			v->push_back(mc2);

			// Key function
			r->mutate(pos1, pos2);

			#ifdef GENETIC_OUTPUT
			printf("interchanges with position %d\n", pos2);
			#endif
		}
	}
}

void genetic::print_best_sol(){
	printf("\nBEST SOLUTION:\n");
	best_sol.print();
	printf("Fitness: %.2f\n", best_sol.get_fitness());
	printf("Obtained in iteration: %d\n", best_it);
}

vector<migration_cell_t> genetic_t::get_pages_to_migrate(map<pid_t, page_table_t> *page_ts){
	// Generates an individual for current state and calculates its performance
	individual ind(*page_ts);
	return do_genetic(ind);
}

// Right now, for simplicity:
vector<migration_cell_t> genetic_t::get_threads_to_migrate(map<pid_t, page_table_t> *page_ts){
	return get_pages_to_migrate(page_ts);
}

