#include "utils.h"

double utils::obtain_double(){
	return (double) rand() / (double) RAND_MAX;
}

bool utils::contains(vector<int> v, int e){
	return find(v.begin(), v.end(), e) != v.end();
}
