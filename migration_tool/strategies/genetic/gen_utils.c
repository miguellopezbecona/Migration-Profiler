#include "gen_utils.h"

double gen_utils::obtain_double(){
	return (double) rand() / (double) RAND_MAX;
}

bool gen_utils::contains(vector<int> v, int e){
	return find(v.begin(), v.end(), e) != v.end();
}
