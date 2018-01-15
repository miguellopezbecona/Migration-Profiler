#include "population.h"

population::population(){
}
    
void population::add(individual idv){
	// If we exceed the size, we erase the oldest element first
	// [TOTHINK]: we could replace the worst individual instead, but that might give problems in update_fitness
	if (v.size() == MAX_SIZE)
		v.erase(v.begin());

    v.push_back(idv);
}
    
void population::set(int idx, individual idv){
    v[idx] = idv;
}

void population::update_fitness(double f){
	// We are assuming we replace the 
	size_t index = v.size() - 1;
    v[index].fitness = f;
}


individual population::get_best_ind(){
	int index = 0;
	for(size_t i=1;i<v.size();i++){
		if(v[i].get_fitness() < v[index].get_fitness()) // Minimization case
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

    for(individual i : v){
        printf("\tIndiv. %d = ", c);
		i.print();
		c++;
    }
}
