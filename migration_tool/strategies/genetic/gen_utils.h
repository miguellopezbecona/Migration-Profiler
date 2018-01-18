#pragma once

#include <algorithm> // find
#include <string> // rand
#include <vector>
using namespace std;

/*** The following macros were meant to be defined in genetic.h, but it raised linking problems ***/
// We will always use MINIMIZATION, but in this way it becomes more general
//#define MAXIMIZATION

#ifndef MAXIMIZATION
	#define MINIMIZATION
#endif

class gen_utils {
	public:
	static int get_rand_int(int max, int no_repeat);
	static double get_rand_double();
	static bool contains(vector<int> v, int e);
};
