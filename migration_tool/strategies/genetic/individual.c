#include "individual.h"

individual::individual() {
}

individual::individual(map<pid_t, page_table_t> ts){
	// [TODO]: this is somewhat inefficient/redundant. A better solution should be achieved

	// For each table (key/first=PID, value/second=table)...
	for(auto const & it : ts){
		pid_t pid = it.first;
		page_table t = it.second;

		// Creates gens based on its page maps
		for(auto const & it2 : t.page_node_map){
			migration_cell_t mc(it2.first, it2.second.current_node, pid, false);
			v.push_back(mc);
		}

		// Creates gens based on its TID maps
		for(auto const & it2 : t.tid_index){
			pid_t tid = it2.first;
			migration_cell_t mc(tid, system_struct_t::get_cpu_from_tid(tid), pid, true);
			v.push_back(mc);
		}
	}
}
    
// To ease copy in cross
individual::individual(vector<migration_cell_t> vec){
	v = vec;
}
    
// Scratch implementation. The real one would use the perf_data from the table
int individual::fitness() const {
	return -1;
}

size_t individual::size(){
	return v.size();
}
    
void individual::set(int index, int value){
	v[index].dest = value;
}

int individual::get(int index){
	return v[index].dest;
}

// Mutation by changing the location directly
migration_cell_t individual::mutate(int index){
	migration_cell *mc = &v[index];
	int new_dest;

	// Different value range depending on type of cell (thread or page address). Avoids repeating current location
	if(mc->is_thread_cell())
		new_dest = gen_utils::get_rand_int(system_struct_t::NUM_OF_CPUS, mc->dest);
	else {
		if(system_struct_t::NUM_OF_MEMORIES == 1) // No possible memory node change. For testing in local only
			return *mc;

		new_dest = gen_utils::get_rand_int(system_struct_t::NUM_OF_MEMORIES, mc->dest);
	}
	mc->dest = new_dest;

	return *mc;
}

// Simple interchange
void individual::mutate(int idx1, int idx2) {
	int aux = v[idx1].dest;
	v[idx1].dest = v[idx2].dest;
	v[idx2].dest = aux;
}
    
// Order crossover
individual individual::cross(individual r, int idx1, int idx2){
	return get_copy();

/*  // [TODO]: this needs a big rewrite
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
*/
}

individual individual::get_copy() {
	return *(new individual(v));    
}
    
void individual::print(){
    printf("0 ");
    for(migration_cell const & mc : v)
        printf("%d ", mc.dest);
}

bool individual::operator < (const individual &other) const {
	return (fitness() < other.fitness());
}
