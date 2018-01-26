#include "population.h"

population::population(){
}
    
void population::add(individual idv){
	// If the population is full, we will replace the first idv with lots of dead TIDs or the one with the worst fitness
	if (v.size() == MAX_SIZE){
		int index = 0;
		for(size_t i=0;i<v.size();i++){
			if(v[i].is_too_old()){
				#ifdef GENETIC_OUTPUT
				printf("Individual %lu is too old, so... ", i);
				#endif

				index = i;
				break;
			}

			#ifdef MAXIMIZATION
			if(v[i].get_fitness() < v[index].get_fitness())
			#elif defined(MINIMIZATION)
			if(v[i].get_fitness() > v[index].get_fitness())
			#endif
				index = i;
		}

		#ifdef GENETIC_OUTPUT
		printf("Individual %d will be replaced.\n", index);
		#endif

		v.erase(v.begin() + index);
	}

	v.push_back(idv); // And then, we add the new idv
}
    
void population::set(int idx, individual idv){
    v[idx] = idv;
}

void population::update_fitness(double f){
	// Only one individual will have NO_FITNESS as fitness, which is the one to have
	for(individual & i : v){
		if(i.get_fitness() == NO_FITNESS){
			i.fitness = f;
			break;
		}
	}
}

individual population::get_best_ind(){
	int index = 0;
	for(size_t i=1;i<v.size();i++){
		#ifdef MAXIMIZATION
		if(v[i].get_fitness() > v[index].get_fitness())
		#elif defined(MINIMIZATION)
		if(v[i].get_fitness() < v[index].get_fitness())
		#endif
			index = i;
	}

	return v[index];
}

bool population::is_first_iteration(){
	return v.empty();
}
    
void population::print(){
	printf("Printing population content:\n");
	int c = 0;

	for(individual const & i : v){
		printf("\tIndiv. %d = ", c);
		i.print();
		c++;
	}
}

