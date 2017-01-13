#ifndef __migration_LIST_H__
#define __migration_LIST_H__

#include <stdio.h>

/* USES tid_list.h */
#include "tid_list.h"

#include <vector>
using namespace std;

/*
 * this list can have tid or free cores, be careful
*/

typedef struct migration_cell{
	tid_cell_t *tid_cell; // NULL if it is a free core. Not checked internaly, beware of inconsistencies.
	int free_core; // -1 if it is a tid. Not checked internaly, beware of inconsistencies.
	int number_of_tickets;

	migration_cell(){}
	migration_cell(tid_cell_t *tid_cell, int core, int number_of_tickets);
	void print();
} migration_cell_t;

typedef struct migration_list{
	vector<migration_cell_t> list;
	int total_tickets;

	migration_list(){
		total_tickets = 0;
	}

	//Use tid_cell=NULL and core>-1 for free core. Use core=-1 for tid.
	int add_cell(tid_cell_t *tid_cell, int core, int number_of_tickets);
	migration_cell_t * get_random_weighted_migration_cell();
	void clear();
	void print();
	bool is_empty();
} migration_list_t;

#endif
