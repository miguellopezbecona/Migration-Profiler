#include "individual.h"

individual::individual() {
}

individual::individual(map<pid_t, page_table_t> ts) : v(system_struct_t::NUM_OF_CPUS, system_struct_t::FREE_CPU) {
	// Builds list from system_struct data
	for(int i=0;i<system_struct_t::NUM_OF_CPUS;i++){
		if(system_struct_t::is_cpu_free(i))
			continue;

		pid_t tid = system_struct_t::get_tid_from_cpu(i);

		// Correct index of CPU in ordered_cpus
		int index = find(system_struct_t::ordered_cpus.begin(), system_struct_t::ordered_cpus.end(), i) - system_struct_t::ordered_cpus.begin();

		v[index] = tid;
	}

	// Calculates fitness (in this case, mean latency)
	// [TODO]: it currently uses all the historical data for the tables, it should just use the data from last iteration
	vector<int> all_ls;
	for(auto const & it : ts){ // For each table (PID)...
		page_table t = it.second;

		for(pid_t const & tid : t.get_tids()){ // Gets all latencies for all pages
			vector<int> l = t.get_lats_for_tid(tid);
			all_ls.insert(end(all_ls), begin(l), end(l));	
		}
	}

	// After all_ls is filled, the mean is calculated:
	this->fitness = accumulate(all_ls.begin(), all_ls.end(), 0.0) / all_ls.size();
}
    
// To ease copy in cross
individual::individual(vector<pid_t> vec){
	v = vec;
}

double individual::get_fitness() const {
	return fitness;
}

size_t individual::size(){
	return v.size();
}
    
void individual::set(int index, ind_type value){
	v[index] = value;
}

ind_type individual::get(int index){
	return v[index];
}

// Simple interchange
void individual::mutate(int idx1, int idx2) {
	pid_t aux = v[idx1];
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
	
	//// Changes what is not the central block ////
	// Gets central block. 

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
    for(pid_t const & tid : v)
        printf("%d ", tid);
}
