#pragma once
#ifndef __GENETIC_UTILS__
#define __GENETIC_UTILS__

#include <algorithm> // find
#include <string> // rand
#include <vector>

/*** The following macros were meant to be defined in genetic.h, but it raised linking problems ***/
// We will always use MINIMIZATION, but in this way it becomes more general
//#define MAXIMIZATION

#ifndef MAXIMIZATION
	#define MINIMIZATION
#endif

class gen_utils {
	public:
	static inline int get_rand_int (const int max, const int no_repeat) {
		int ret = no_repeat;
		while (ret == no_repeat)
			ret = rand() % max;
		return ret;
	}

	static inline double get_rand_double () {
		return (double) rand() / (double) RAND_MAX;
	}

	static inline bool contains(const std::vector<int> & v, const int e) {
		return find(v.begin(), v.end(), e) != v.end();
	}
};

#endif
