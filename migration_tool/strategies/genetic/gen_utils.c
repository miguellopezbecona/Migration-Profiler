#include "gen_utils.h"

// From 0 to max. no_repeat can be used to avoid returning a determined int
int gen_utils::get_rand_int(int max, int no_repeat){
	int ret = no_repeat;
	while(ret == no_repeat)
		ret = rand() % max;
	return ret;
}

// From 0.0 to 1.0
double gen_utils::get_rand_double(){
	return (double) rand() / (double) RAND_MAX;
}

bool gen_utils::contains(vector<int> v, int e){
	return find(v.begin(), v.end(), e) != v.end();
}
