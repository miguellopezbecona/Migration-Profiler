#include "individual.h"

individual::individual() {
	fitness = NO_FITNESS; // Unknown potential
}

individual::individual(map<pid_t, page_table_t> ts) : v(system_struct_t::NUM_OF_CPUS) {
	// Builds list from system_struct data
	for(int i=0;i<system_struct_t::NUM_OF_CPUS;i++){
		if(system_struct_t::is_cpu_free(i))
			continue;

		pid_t tid = system_struct_t::get_tid_from_cpu(i);

		// Correct index of CPU in ordered_cpus
		int index = find(system_struct_t::ordered_cpus.begin(), system_struct_t::ordered_cpus.end(), i) - system_struct_t::ordered_cpus.begin();

		v[index].push_back(tid);
	}

	// Calculates fitness (in this case, mean latency), using only data from last iteration
	vector<int> all_ls;
	for(auto & it : ts){ // For each table (PID), gets all its latencies
		vector<int> l = it.second.get_all_lats();
		all_ls.insert(end(all_ls), begin(l), end(l));
	}

	// After all_ls is filled, the mean is calculated:
	this->fitness = accumulate(all_ls.begin(), all_ls.end(), 0.0) / all_ls.size();
}
    
// To ease copy in cross
individual::individual(vector<gene> vec){
	v = vec;
	fitness = NO_FITNESS; // Unknown potential
}

double individual::get_fitness() const {
	return fitness;
}

size_t individual::size(){
	return v.size();
}
    
void individual::set(int index, gene value){
	v[index] = value;
}

bool individual::is_too_old(){
	const double PERC_THRES = 0.5;

	unsigned short num_tids = 0;
	unsigned short dead_tids = 0;

	// Check the percentage of dead TIDs the idv has
	for(gene const & tids : v){
		for(pid_t const & tid : tids){
			num_tids++;

			pid_t pid = system_struct_t::get_pid_from_tid(tid);
			if(!is_tid_alive(pid,tid))
				dead_tids++;
		}
	}

	double perc = dead_tids / num_tids;
	return perc >= PERC_THRES;
}

// Simple interchange
void individual::mutate(int idx1, int idx2) {
	gene aux = v[idx1];
	v[idx1] = v[idx2];
	v[idx2] = aux;
}

// Order crossover
individual individual::cross(individual other, int idx1, int idx2){
	size_t sz = v.size();
	int cut1, cut2, copy_idx;

	gene num;
	
	// Gets which one is the first cut and which is the second
	if(idx1 > idx2){
		cut1 = idx2;
		cut2 = idx1;
	} else {
		cut1 = idx1;
		cut2 = idx2;
	}

	// We will only allow a maximum of free CPUs (repeated values)
	short free_cpus = count(v.begin(), v.end(), *(new gene()) );
	short free_cpus_other = count(other.v.begin(), other.v.end(), *(new gene()) );
	if(free_cpus_other > free_cpus)
		free_cpus = free_cpus_other;
	
	// Creates a copy which will already have the center block copied
	individual son = get_copy();

	// Gets central block from current individual
	std::set<gene> sublist(v.begin() + cut1, v.begin() + cut2+1);
	
	//// Changes what is not the central block ////
	
	// Copies from second cut until the end from the other individual
	copy_idx = (cut2+1) % sz;
	for(size_t i=cut2+1;i<sz;i++){
		// If the generated index is already part of the sublist, then we continue to the next in a circular manner
		do {
			num = other[copy_idx];
			copy_idx = (copy_idx + 1) % sz;

			// We will only allow a maximum of free CPUs (repeated values)
			if(num.empty() && free_cpus){
				free_cpus--;
				break;
			}
		} while(sublist.count(num)); // sublist.contains(num)
		sublist.insert(num);
		son.set(i, num);
	}
	
	// Copies from start to second cut
	for(int i=0;i<cut1;i++){
		do {
			num = other[copy_idx];
			copy_idx++;

			// We will only allow a maximum of free CPUs (repeated values)
			if(num.empty() && free_cpus){
				free_cpus--;
				break;
			}	
		} while(sublist.count(num));
		sublist.insert(num);
		son.set(i, num);
	}
	
	return son;
}

individual individual::get_copy() {
	individual i = *(new individual(v));
	return i;
}
    
void individual::print() const {
	printf("{fitness: %.2f, content: ", fitness);
	for(gene const & tids : v){
		if(tids.empty()){
			printf("F ");
			continue;
		}

		auto &last = *(--tids.end());
		for(pid_t const & tid : tids){
			printf("%d", tid);
			if (&tid != &last)
				printf("_");
		}
		printf(" ");
	}
	printf("}\n");
}

gene individual::operator[](int i) const {
	return v[i];
}

gene & individual::operator[](int i) {
	return v[i];
}
