#include "population.h"

population::population(){
}
    
void population::add(individual r){
	// Inserts circularly. [TODO]: if we exceed the capacity, we can replace the worst individual instead
	if (v.size() == MAX_SIZE)
		v.erase(v.end());

    v.push_back(r);
}
    
void population::set(int index, individual r){
    v[index] = r;
}

individual population::get_best_ind(){
	int index = 0;
	for(size_t i=1;i<v.size();i++){
		if(v[i].get_fitness() < v[index].get_fitness()) // Minimization case
			index = i;
	}

    return v[index];
}
    
void population::print(){
	printf("Printing population content:\n");
	int c = 0;

    for(individual i : v){
        printf("\tInd %d = {fitness: %.2f, content: ", c, i.get_fitness());
		i.print();
		printf("}\n");
		c++;
    }
}
