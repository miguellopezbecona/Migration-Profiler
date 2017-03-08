#include "population.h"

population::population(){
}
    
void population::add(individual r){
	// Inserts circularly
	if (v.size() == MAX_SIZE)
		v.erase(v.end());

    v.push_back(r);
}
    
void population::set(int index, individual r){
    v[index] = r;
}
    
void population::customSort(){
    sort(v.begin(), v.end());
}
    
// If the population is ordered by fitness, the best will be the first
individual population::get_best_ind(){
    return v[0];
}
    
void population::print(){
	printf("Printing population content:\n");
	int c = 0;

    for(individual i : v){
        printf("\tInd %d = {fitness: %d, content: ", c, i.fitness());
		i.print();
		printf("}\n");
		c++;
    }
}
