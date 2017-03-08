#include "individual.h"

individual::individual() {
}
    
// To ease copy
individual::individual(vector<int> vec){
	v = vec;
}
    
// Scratch implementation. The real one would use the perf_data from the table
int individual::fitness() const {
	int s = 0;
	for(int v : v)
		s += v;
	return s;
}

size_t individual::size(){
	return v.size();
}
    
void individual::add(int value){
	v.push_back(value);
}
    
void individual::set(int index, int value){
	v[index] = value;
}

int individual::get(int index){
	return v[index];
}
    
bool individual::contains(int city){
	return find(v.begin(), v.end(), city) != v.end();
}
    
void individual::mutate(int idx1, int idx2) {
	int aux = v[idx1];
	v[idx1] = v[idx2];
	v[idx2] = aux;
}
    
// Order crossover
individual individual::cross(individual r, int idx1, int idx2){
	int cut1, cut2, copy_idx, num;
	
	// Gets which is the first cut and which is the second
	if(idx1 > idx2){
		cut1 = idx2;
		cut2 = idx1;
	} else {
		cut1 = idx1;
		cut2 = idx2;
	} 
	
	// Creates a copy which will already have the center block copied
	individual son = get_copy();
	
	/*** Changes what is not the central block ***/
	// Gets central block
	vector<int> sublist(v.begin() + cut1, v.begin() + cut2+1);
	
	// Copies from second cut until the end
	copy_idx = (cut2+1) % v.size();
	for(int i=cut2+1;i<v.size();i++){
		// Si el índice generado ya forma parte de la sublist, se continúa al siguiente de forma circular
		do {
			num = r.get(copy_idx);
			copy_idx = (copy_idx + 1) % v.size();
		} while(find(sublist.begin(), sublist.end(), num) != sublist.end()); // sublist.contains(num)
		sublist.push_back(num);
		son.set(i, num);
	}
	
	// Copies from start to second cut
	for(int i=0;i<cut1;i++){
		do {
			num = r.get(copy_idx);
			copy_idx = (copy_idx + 1) % v.size();
		} while(find(sublist.begin(), sublist.end(), num) != sublist.end()); // sublist.contains(num)
		sublist.push_back(num);
		son.set(i, num);
	}
	
	return son;
}

individual individual::get_copy() {
	return *(new individual(v));    
}
    
void individual::print(){
    printf("0 ");
    for(int const & el : v)
        printf("%d ", el);
}

bool individual::operator < (const individual &other) const {
	return (fitness() < other.fitness());
}
