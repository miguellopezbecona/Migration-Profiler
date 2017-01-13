#include "migration_list.h"

/*** migration_cell_t functions ***/
// Constructor
migration_cell_t::migration_cell(tid_cell_t *tid_cell,int core, int number_of_tickets){
	this->tid_cell = tid_cell;
	this->free_core = core;
	this->number_of_tickets = number_of_tickets;
}

void migration_cell_t::print(){
	printf("With %d tickets!  ", number_of_tickets);
	if(free_core==-1){
		tid_cell->print();
	}else{
		printf("Free core %d\n", free_core);
	}
}

/*** migration_list_t functions ***/
int migration_list_t::add_cell(tid_cell_t *tid_cell, int core, int number_of_tickets){
	migration_cell_t cell(tid_cell, core, number_of_tickets);
	list.push_back(cell);
	total_tickets += cell.number_of_tickets;

	return 0;
}

migration_cell_t* migration_list_t::get_random_weighted_migration_cell(){
	int result = rand() % total_tickets;

	migration_cell_t *cell;	
	for(int i=0;i<list.size();i++){
		cell = &list[i];
		result = result - cell->number_of_tickets;
		if(result < 0){
			return cell;
		}
	}

	//should never be reached
	return NULL;
}

void migration_list_t::clear(){
	list.clear();
}

void migration_list_t::print(){
	if(list.empty()){
		printf("migration list empty\n");
		return;
	}

	printf("migration list, total tickets %d\n", total_tickets);

	for(int i=0;i<list.size();i++)
		list[i].print();
}

bool migration_list_t::is_empty(){
	return list.empty();
}

