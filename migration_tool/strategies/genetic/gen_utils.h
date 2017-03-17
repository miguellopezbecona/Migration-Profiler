#pragma once

#include <algorithm> // find
#include <string> // rand
#include <vector>

using namespace std;

class gen_utils {
	public:
	static int get_rand_int(int max, int no_repeat);
	static double get_rand_double();
	static bool contains(vector<int> v, int e);
};
