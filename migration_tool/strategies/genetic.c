#include "genetic.h"

genetic::genetic() : best_sol() {
	srand(time(NULL)); // Changes rand seed
	it = 0;
	best_it = 0;
}

// Core function that executes one iteration of all the genetic processes
vector<migration_cell_t> genetic::do_genetic(individual ind){
	vector<migration_cell_t> v; // Holds migrations generated by genetic processes

	it++;

	#ifdef GENETIC_OUTPUT
	printf("Begin of iteration %d of genetic process.\n", it);
	#endif

	/** Tournament, cross, mutation, and replacement **/

	// If there are no other individuals, only mutation will be done
	individual child = ind;
	if(!p.is_first_iteration()){
		p.update_fitness(ind.get_fitness());

		individual chosen = tournament();
		child = cross(ind, chosen, &v);
	}
	mutation(&child, &v);
	p.add(child); // Final result is appended to population with replacement

	#ifdef GENETIC_OUTPUT
	printf("End of iteration %d. ", it);
	p.print(); // Prints poblation content
	#endif

	individual best = p.get_best_ind();

	// Updates best_sol if needed
	#ifdef MAXIMIZATION
	if(best.get_fitness() > best_sol.get_fitness()){
	#elif defined(MINIMIZATION)
	if(best.get_fitness() < best_sol.get_fitness()){
	#endif
		best_sol = best;
		best_it = it;
	}


	#ifdef GENETIC_OUTPUT
	printf("\n");
	#endif

	return v;
}

// In this simplified version, we only select one individual (the one to be crossed with the current individual)
// so this can be implemented simply as picking the best (lowest fitness)
individual genetic::tournament(){
	individual ind = p.get_best_ind();

	#ifdef GENETIC_OUTPUT
	printf("Winner idv. from selection: ");
	ind.print();
	#endif

	return ind;
}
    
// Crosses tournament winner with current individual
individual genetic::cross(individual from_iter, individual from_selec, vector<migration_cell_t> *v){
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
		int idx1 = gen_utils::get_rand_int(sz, -1); // Any possible index
		int idx2 = gen_utils::get_rand_int(sz, idx1); // The second index can't be the first one...

		// Two descendents can be obtained with order crossover
		individual c1 = from_selec.cross(from_iter, idx1, idx2);
		individual c2 = from_iter.cross(from_selec, idx1, idx2);

		#ifdef GENETIC_OUTPUT
		printf("index cuts: %d %d\n", idx1, idx2);
		#endif

		// We can only keep one child, decided randomly
		prob = gen_utils::get_rand_double();
		individual chosen_child;
		if(prob > 0.5){
			#ifdef GENETIC_OUTPUT
			printf("\tWe kept first child: ");
			#endif
			chosen_child = c1;
		} else {
			#ifdef GENETIC_OUTPUT
			printf("\tWe kept second child: ");
			#endif
			chosen_child = c2;
		}

		#ifdef GENETIC_OUTPUT
		chosen_child.print();
		#endif

		// We generate migration cells seeing the differences between the idvs from the iteration and the selected son
		for(size_t i=0;i<sz;i++){
			if(from_iter[i] != chosen_child[i]){ // Difference: TIDs from child are migrated to its CPU possition
				for(pid_t tid : chosen_child[i]){
					migration_cell_t mc(tid, system_struct_t::ordered_cpus[i]);
					v->push_back(mc);
				}
			}
		}

		return chosen_child;
	}
}
       
// Mutates current individual
void genetic::mutation(individual *ind, vector<migration_cell_t> *v){
	#ifdef GENETIC_OUTPUT
	printf("Mutation process:\n");
	#endif

	size_t sz = ind->size();
	for(size_t pos1=0;pos1<sz;pos1++){

		// Obtains probability
		double prob = gen_utils::get_rand_double();

		#ifdef GENETIC_OUTPUT
		printf("\tIndex: %lu (rand rum %.2f) -> ", pos1, prob);
		#endif

		if(prob > MUT_PROB){
			#ifdef GENETIC_OUTPUT
			printf("mutation not done\n");
			#endif
		} else {
			// Mutation by interchange: selects second index
			int pos2 = gen_utils::get_rand_int(sz, pos1);

			// Generates two migration cells: ind[pos1] goes to ind[pos2] location and viceversa
			int dest1 = system_struct_t::ordered_cpus[pos2]; // CPU associated to pos2
			int dest2 = system_struct_t::ordered_cpus[pos1];

			// Migration cells concerning to those TIDs may already exist from cross process. If this is the case, we just have to change their destination
			bool first_exists = false, sec_exists = false;
			for(size_t i=0; i<v->size();i++){
				migration_cell_t *mc = &v->operator[](i);
				for(pid_t const & tid : ind->v[pos1]){
					if(mc->elem == tid){ // Cell already exists: changes its destination
						mc->dest = dest2;
						first_exists = true;
					}
				}

				for(pid_t const & tid : ind->v[pos2]){
					if(mc->elem == tid){ // Cell already exists: changes its destination
						mc->dest = dest1;
						sec_exists = true;
					}
				}
			}

			// It won't generate migration cells for free CPUs positions (empty arrays) or existing cells
			if(!first_exists){
				for(pid_t const & tid : ind->v[pos1]){
					migration_cell_t mc1(tid, dest1);
					v->push_back(mc1);
				}
			}
			if(!sec_exists){
				for(pid_t const & tid : ind->v[pos2]){
					migration_cell_t mc2(tid, dest2);
					v->push_back(mc2);
				}
			}

			// Key function
			ind->mutate(pos1, pos2);

			#ifdef GENETIC_OUTPUT
			printf("interchanges with position %d\n", pos2);
			#endif
		}
	}
}

void genetic::print_best_sol(){
	printf("\nBEST SOLUTION: ");
	best_sol.print();
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

