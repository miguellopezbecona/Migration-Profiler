#include "rapl.h"

energy_data_t ed;

int init_energy_things() {
	return ed.prepare_energy_data();
}

void read_energy_data(double seconds_sleep){
	ed.read_buffer(seconds_sleep);
}

void clean_energy_end() {
	ed.close_buffers();
}

