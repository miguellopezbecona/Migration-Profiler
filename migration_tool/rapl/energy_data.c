#include "energy_data.h"

vector<char*> energy_data_t::rapl_domain_names;
int energy_data_t::NUM_RAPL_DOMAINS = 0;

bool comparison_func(const char *c1, const char *c2){
	return strcmp(c1, c2) < 0;
}

// Frees everything
energy_data_t::~energy_data(){
	// Nothing was initialized
	if(curr_vals == NULL)
		return;

	for(int i=0; i<system_struct_t::NUM_OF_MEMORIES; i++){
		free(base_vals[i]);
		free(prev_vals[i]);
		free(curr_vals[i]);
		free(fd[i]);
	}

	for(int i=0; i<NUM_RAPL_DOMAINS; i++)
		free(units[i]);
	rapl_domain_names.clear();

	free(base_vals);
	free(prev_vals);
	free(curr_vals);
	free(units);
	free(fd);
	free(scale);
}

void energy_data_t::detect_domains(){
	struct dirent *buffer = NULL;
	DIR *dir = NULL;
	const char* folder = "/sys/bus/event_source/devices/power/events";

	if( (dir=opendir(folder)) == NULL)
		return;

	while( (buffer = readdir(dir)) !=NULL){
		if(buffer->d_name[0] == '.') // No self-directory/parent
			continue;

		bool is_domain_name = true;

		// Equal to filename but without "energy-"
		char no_energy[strlen(buffer->d_name)-7];
		sscanf(buffer->d_name, "energy-%s", no_energy);

		// Domain names don't have a "." in their filename. All start with "energy-xxx", so we don't have to start in i=0
		for(int i=10;buffer->d_name[i] != '\0'; i++) {
			if(buffer->d_name[i] == '.'){
				is_domain_name = false;
				break;
			}
		}

		if(is_domain_name)
			rapl_domain_names.push_back(strdup(no_energy));
	}

	sort(rapl_domain_names.begin(), rapl_domain_names.end(), comparison_func); // Not necessary, but meh

	#ifdef INIT_VERBOSE
	printf("RAPL domains detected: %d. They are:", NUM_RAPL_DOMAINS);
	for(char* const & d_name : rapl_domain_names)
		printf(" %s", d_name);
	printf("\n");
	#endif

	closedir(dir);
}

void energy_data_t::allocate_data(){
	base_vals = (double**)malloc(system_struct_t::NUM_OF_MEMORIES*sizeof(double*));
	prev_vals = (double**)malloc(system_struct_t::NUM_OF_MEMORIES*sizeof(double*));
	curr_vals = (double**)malloc(system_struct_t::NUM_OF_MEMORIES*sizeof(double*));
	units = (char**)malloc(NUM_RAPL_DOMAINS*sizeof(char*));
	fd = (int**)malloc(system_struct_t::NUM_OF_MEMORIES*sizeof(int*));
	scale = (double*)malloc(NUM_RAPL_DOMAINS*sizeof(double));

	for(int i=0; i<system_struct_t::NUM_OF_MEMORIES; i++){
		base_vals[i] = (double*)calloc(NUM_RAPL_DOMAINS, sizeof(double));
		prev_vals[i] = (double*)calloc(NUM_RAPL_DOMAINS, sizeof(double));
		curr_vals[i] = (double*)malloc(NUM_RAPL_DOMAINS*sizeof(double));
		fd[i] = (int*)malloc(NUM_RAPL_DOMAINS*sizeof(int));
	}

	for(int i=0; i<NUM_RAPL_DOMAINS; i++)
		units[i] = (char*)malloc(8*sizeof(char));
}

int energy_data_t::read_increments_file(char* base_filename){
	FILE *file = fopen(base_filename, "r");
	if(file == NULL){
		printf("Base energy consumptions could not be read. Filename was: %s\n", base_filename);
		return -1;
	}

	// For skipping header line
	char* buffer = (char*)malloc(32*sizeof(char));
	buffer = fgets(buffer, 32, file);

	int node;
	double val;

	while(true){
		int ret = fscanf(file, "%d,%[^,],%lf", &node, buffer, &val);
		if(ret < 3)
			break;

		int col = get_domain_pos(buffer);
		base_vals[node][col] = val;
	}
	fclose(file);

	return 0;
}

int energy_data_t::prepare_energy_data(char* base_filename){
	FILE *fff;
	int type;
	char filename[BUFSIZ];

	// Fixed domains in this case
	//rapl_domain_names.push_back((char *) "pkg");
	//rapl_domain_names.push_back((char *) "ram");
	detect_domains();

	NUM_RAPL_DOMAINS = rapl_domain_names.size();

	allocate_data();

	fff=fopen("/sys/bus/event_source/devices/power/type","r");
	if (fff==NULL) {
		printf("\tNo perf_event rapl support found (requires Linux 3.14)\n");
		printf("\tFalling back to raw msr support\n\n");
		return -1;
	}
	int dummy = fscanf(fff,"%d",&type);
	fclose(fff);

	if(dummy != 1)
		return -1;

	int config[NUM_RAPL_DOMAINS];

	// Gets data to open counters
	for(int i=0;i<NUM_RAPL_DOMAINS;i++) {
		sprintf(filename,"/sys/bus/event_source/devices/power/events/energy-%s", rapl_domain_names[i]);
		fff=fopen(filename,"r");

		if (fff!=NULL) {
			dummy = fscanf(fff,"event=%x",&config[i]);
			fclose(fff);
		} else {
			printf("Error while opening config file for domain %s.\nThis is probably due to that domain not being supported. It will be removed from the list.\n", rapl_domain_names[i]);
			rapl_domain_names.erase(rapl_domain_names.begin() + i);
			NUM_RAPL_DOMAINS--;
			continue;
		}

		sprintf(filename,"/sys/bus/event_source/devices/power/events/energy-%s.scale", rapl_domain_names[i]);
		fff=fopen(filename,"r");

		if (fff!=NULL) {
			dummy = fscanf(fff,"%lf",&scale[i]);
			fclose(fff);
		}

		sprintf(filename,"/sys/bus/event_source/devices/power/events/energy-%s.unit", rapl_domain_names[i]);
		fff=fopen(filename,"r");

		if (fff!=NULL) {
			dummy = fscanf(fff,"%s",units[i]);
			fclose(fff);
		}
	}

	read_increments_file(base_filename);

	// Opens counters
	for(int n=0;n<system_struct_t::NUM_OF_MEMORIES;n++) {
		for(int d=0;d<NUM_RAPL_DOMAINS;d++) {
			fd[n][d] = -1;

			struct perf_event_attr attr;
			memset(&attr,0x0,sizeof(attr));
			attr.type = type;
			attr.config = config[d];
			if (config[d]==0) continue;

			fd[n][d] = perf_event_open(&attr,-1, system_struct_t::node_cpu_map[n][0],-1,0);
			if(fd[n][d] < 0) {
				printf("\tError code while opening buffer for CPU %d, config %d: %d\n\n", system_struct_t::node_cpu_map[n][0], config[d], fd[n][d]);
				return -1;
			}
		}
	}

	return 0;
}

// Since rapl_domain_names will always be a very small array with short strings, a map and/or special comparator won't be used
int energy_data_t::get_domain_pos(const char* domain){
	for(int i=0;i<NUM_RAPL_DOMAINS;i++){
		if(strcmp(rapl_domain_names[i], domain) == 0)
			return i;
	}
	return -1;
}

void energy_data_t::read_buffer(double secs) {
	long long value;

	for(int n=0;n<system_struct_t::NUM_OF_MEMORIES;n++) {
		for(int d=0;d<NUM_RAPL_DOMAINS;d++) {
			int dummy = read(fd[n][d],&value,8);

			// Can be uncommented to avoid a "unused-variable" warning, but I think it may affect performance
			/*
			if(dummy < 0)
				//return;
			*/
			double raw_value_scaled = ((double)value*scale[d]);
			curr_vals[n][d] = (raw_value_scaled - prev_vals[n][d]) / secs - base_vals[n][d]; // Normal and base increments
			prev_vals[n][d] = raw_value_scaled;

			// If a negative value is raised due to varying base value, this is updated and the current is nullified
			if(curr_vals[n][d] < 0.0){
				base_vals[n][d] += curr_vals[n][d];
				curr_vals[n][d] = 0.0;
			}
		}
	}
}

double energy_data_t::get_curr_val(int node, const char* domain) {
	int pos = get_domain_pos(domain);

	if(pos < 0) // Not found
		return 0.0;
	else
		return curr_vals[node][pos];
}

// For all domains
vector<double> energy_data_t::get_curr_vals_from_node(int node) {
	vector<double> v;

	for(int i=0;i<NUM_RAPL_DOMAINS;i++)
		v.push_back(curr_vals[node][i]);

	return v;
}

// For all nodes
vector<double> energy_data_t::get_curr_vals_from_domain(const char* domain) {
	vector<double> v;
	int pos = get_domain_pos(domain);

	if(pos == -1)
		return v;

	for(int i=0;i<system_struct_t::NUM_OF_MEMORIES;i++)
		v.push_back(curr_vals[i][pos]);

	return v;
}

// Everything
double** energy_data_t::get_curr_vals() {
	return curr_vals;
}

double energy_data_t::get_ratio_against_base(double val, int node, const char* domain){
	int col = get_domain_pos(domain);
	return val / base_vals[node][col];
}

void energy_data_t::print_curr_vals() {
	for(int n=0;n<system_struct_t::NUM_OF_MEMORIES;n++) {
		printf("Node %d:\n", n);

		for(int d=0;d<NUM_RAPL_DOMAINS;d++)
			printf("\tEnergy consumed in %s: %lf %s\n", rapl_domain_names[d], curr_vals[n][d], units[d]);
	}
	printf("\n");
}

void energy_data_t::close_buffers() {
	for(int i=0;i<system_struct_t::NUM_OF_MEMORIES;i++) {
		for(int j=0;j<NUM_RAPL_DOMAINS;j++)
			close(fd[i][j]);
	}
}

energy_data_t ed;

