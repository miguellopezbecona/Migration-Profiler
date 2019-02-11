#pragma once
#ifndef __POPULATION__
#define __POPULATION__

#include "individual.hpp"

const int MAX_SIZE = 5;

class population {
public:
	std::vector<individual_t> v;

	population () :
		v()
	{
		v.reserve(MAX_SIZE);
	};

	void add (const individual_t idv) {
		// If the population is full, we will replace the first idv with lots of dead TIDs or the one with the worst fitness
		if (v.size() == MAX_SIZE) {
			size_t index = 0;
			for (size_t i = 0; i < v.size(); i++) {
				if (v[i].is_too_old()) {
					#ifdef GENETIC_OUTPUT
					std::cout << "Individual " << i << " is too old, so... ";
					#endif

					index = i;
					break;
				}

				#ifdef MAXIMIZATION
				if (v[i].get_fitness() < v[index].get_fitness())
				#elif defined(MINIMIZATION)
				if (v[i].get_fitness() > v[index].get_fitness())
				#endif
					index = i;
			}

			#ifdef GENETIC_OUTPUT
			std::cout << "Individual " << index << " will be replaced." << '\n';
			#endif

			v.erase(v.begin() + index);
		}

		v.push_back(idv); // And then, we add the new idv
	}

	inline void set (const size_t idx, const individual_t & idv) {
	    v[idx] = idv;
	}

	inline void update_fitness (const double fitness) { // A posteriori performance update the latest-1_th state
		// Only one individual will have NO_FITNESS as fitness, which is the one to modify

		// Since we always replace only one element and we insert at the end, the element to modify is the last one. If not, uncomment the next block
		v.back().fitness = fitness;
	}

	inline individual_t get_best_ind () const {
		size_t index = 0;

		for (size_t i = 1; i < v.size(); i++) {
			#ifdef MAXIMIZATION
			if (v[i].get_fitness() > v[index].get_fitness())
			#elif defined(MINIMIZATION)
			if (v[i].get_fitness() < v[index].get_fitness())
			#endif
				index = i;
		}

		return v[index];
	}

	inline bool is_first_iteration () const {
		return v.empty();
	}

	inline void print () const {
		std::cout << *this << '\n';
	}

	friend std::ostream & operator << (std::ostream & os, const population & p) {
		os << "Printing population content:" << '\n';
		size_t c = 0;

		for (individual_t const & i : p.v) {
			os << "Indiv. " << c << " = " << i << '\n';
			c++;
		}

		return os;
	}
};

#endif
