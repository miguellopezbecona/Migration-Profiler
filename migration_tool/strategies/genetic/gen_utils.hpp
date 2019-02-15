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

namespace gen_utils {
	inline int get_rand_int (const int max, const int no_repeat) {
		auto ret = no_repeat;
		do {
			ret = rand() % max;
		} while (ret == no_repeat);
		return ret;
	}

	inline double get_rand_double () {
		return static_cast<double>(rand()) / RAND_MAX;
	}

	template<class T>
	inline bool contains (const std::vector<T> & v, const T e) {
		return find(v.begin(), v.end(), e) != v.end();
	}
};

#endif
