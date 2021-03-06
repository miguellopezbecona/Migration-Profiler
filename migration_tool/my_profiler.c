/*
 * Based on syst_smpl.c - example of a system-wide sampling
 *
 * Copyright (c) 2010 Google, Inc
 * Contributed by Stephane Eranian <eranian@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <signal.h>
#include <getopt.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "perfmon/perf_util.h" // transfer_data_from_buffer_to_structure
#include "utils.h" // For time utils
#include "sample_data.h"
#include "migration/migration_facade.h" // begin_migration_process
#include "rapl/energy_data.h" // energy stuff

// Using a value greater than 2 requires additional changes in, at least, "events" and "periods" array
#define NUM_GROUPS 2

// Uncomment the following for testing functionalities without using the hardware counters
//#define FAKE_DATA

//#define EVENT_OUTPUT
//#define ABORT_IF_MANY_UNKNOWN

#if defined(JUST_PROFILE_ENERGY) || defined(USE_ENER_ST)
	#define USE_ENERGY
#endif

typedef struct {
	char* base_filename;

	int sbm;
	int periods[NUM_GROUPS];
	int minimum_latency;
} options_t;

int uid; // For PID filtering

static uint64_t collected_samples_group[NUM_GROUPS], lost_samples_group[NUM_GROUPS], processed_samples_group[NUM_GROUPS];
static uint64_t buffer_reads[NUM_GROUPS];
uint64_t unknown_samples = 0;

static perf_event_desc_t **all_fds[NUM_GROUPS];
static int num_fds[NUM_GROUPS];
static options_t options;
static size_t map_size;

time_t last_migr_time;

static const char *events[2] = {
	//"RAPL_ENERGY_PKG:period=1000",
	"MEM_TRANS_RETIRED:LATENCY_ABOVE_THRESHOLD:period=1000"
	,"INST_RETIRED:period=1000000,OFFCORE_REQUESTS:ALL_DATA_RD" // Inst, REQ_DR
	//,"INSTRUCTIONS:period=10000000,FP_COMP_OPS_EXE:SSE_SCALAR_DOUBLE,LAST_LEVEL_CACHE_MISSES,OFFCORE_REQUESTS:ALL_DATA_READ" // Inst, double FLOPS, LLC fails, REQ_DR
	//,"INSTRUCTIONS:period=10000000,FP_COMP_OPS_EXE:SSE_SCALAR_DOUBLE,FP_COMP_OPS_EXE:SSE_FP_SCALAR_SINGLE,FP_COMP_OPS_EXE:SSE_FP_PACKED_DOUBLE,OFFCORE_REQUESTS:ALL_DATA_READ" // Inst, some kinds of FLOPS, REQ_DR
	//,"INSTRUCTIONS:period=10000000,FP_COMP_OPS_EXE:SSE_SCALAR_DOUBLE,FP_COMP_OPS_EXE:SSE_FP_SCALAR_SINGLE,FP_COMP_OPS_EXE:SSE_FP_PACKED_DOUBLE,FP_COMP_OPS_EXE:SSE_PACKED_SINGLE" // Inst, all kind of FLOPS
};

static void clean_end(int n);

#ifndef JUST_PROFILE
void perform_migration(){
	// Time is got and energy buffers are read
	time_t current_time = time(NULL);
	double secs = difftime(current_time, last_migr_time);

	// We profile every 1, 2 or 4 seconds depending on get_time_value()
	if(secs > (get_time_value() * inv_1000)){
		last_migr_time = current_time;
		//printf("\n***********\nAt %s\n",ctime(&last_migr_time));
		begin_migration_process();
	}

}
#endif

static void process_smpl_buf(perf_event_desc_t *hw, int cpu, perf_event_desc_t **all_fds_p, int num_fds_p, int name) {

	struct perf_event_header ehdr;
	perf_event_desc_t *fds = NULL;

	fds = all_fds_p[cpu];

	for(int i=0;;i++) {
		int ret = perf_read_buffer(hw, &ehdr, sizeof(ehdr));
		if (ret) { // Nothing left to read
			#ifdef JUST_PROFILE_ENERGY
			if(i!=0) // // Adds energy data to last sample, if applicable (at least a sample (i!=0) was read)
				add_energy_data_to_last_sample();
			#endif

			return;
		}

		switch(ehdr.type) {
			case PERF_RECORD_SAMPLE: {
				my_pebs_sample_t my_sample;
				my_sample.values = (uint64_t*)malloc(sizeof(uint64_t)*num_fds_p);

				ret = transfer_data_from_buffer_to_structure(fds, num_fds_p, hw - fds, &ehdr, &my_sample, uid);
				processed_samples_group[name]++;

				if(ret < 0) // PID from process we can't migrate so it will be skipped
					break;

				add_data_to_list(my_sample);

				#ifndef JUST_PROFILE
				free(my_sample.values); // Not needed anymore at this point
				#endif
				
				if (ret)
					errx(1, "cannot parse sample");

				collected_samples_group[name]++;
			}
				break;
			case PERF_RECORD_EXIT:
				//display_exit(hw, options.output_file);
				break;
			case PERF_RECORD_LOST:
				lost_samples_group[name] += display_lost(hw, fds, num_fds_p, stderr);
				break;
			case PERF_RECORD_THROTTLE:
				//display_freq(1, hw, options.output_file);
				break;
			case PERF_RECORD_UNTHROTTLE:
				//display_freq(0, hw, options.output_file);
				break;
			default:
				// Skips different entire map page or partial
				size_t to_skip = ehdr.size == 0 ? map_size : ehdr.size - sizeof(ehdr);
				//printf("Unknown sample type %d, skipping %lu bytes.\n", ehdr.type, to_skip);

				perf_skip_buffer(hw, to_skip);

				unknown_samples++;

				#ifdef ABORT_IF_MANY_UNKNOWN
				if(unknown_samples > 50){
					printf("Error while reading buffer, too many unknown samples, exiting...\n");
					clean_end(-1);
					exit(-1);
				}
				#endif

				break;
		}
		
	}
}

int setup_cpu(int cpu, int fd, int group) {
	perf_event_desc_t *fds = NULL;
	int ret, flags;
	size_t pgsz = sysconf(_SC_PAGESIZE);
	const int mmap_pages = 16;
	map_size = (mmap_pages+1)*pgsz;


	// Allocate fds
	ret = perf_setup_list_events(events[group], &fds, &num_fds[group]);
	if (ret || !num_fds[group])
		errx(1, "cannot setup event list");

	all_fds[group][cpu] = fds;
	
	// Here we define special configuration for each group
	if(group == 0){ // Memory
		//Config for latency monitoring
		fds[0].hw.config1 = options.minimum_latency;
		fds[0].hw.precise_ip = 2;
	}
	fds[0].hw.sample_period	= options.periods[group];

	if (!fds[0].hw.sample_period)
		errx(1, "need to set sampling period or freq on first event, use :period= or :freq=");

	fds[0].fd = -1;
	for(int i=0; i < num_fds[group]; i++) {
		fds[i].hw.disabled = !i; // start immediately

		flags = 0;

		if (fds[i].hw.sample_period) {
			// set notification threshold to be halfway through the buffer
			if (fds[i].hw.sample_period) {
				fds[i].hw.wakeup_watermark = (mmap_pages*pgsz) / 2;
				fds[i].hw.watermark = 1;
			}

			fds[i].hw.sample_type = PERF_SAMPLE_IP|PERF_SAMPLE_TID|PERF_SAMPLE_READ|PERF_SAMPLE_TIME|PERF_SAMPLE_PERIOD|PERF_SAMPLE_STREAM_ID|PERF_SAMPLE_ADDR|PERF_SAMPLE_CPU|PERF_SAMPLE_WEIGHT|PERF_SAMPLE_DATA_SRC;
			#ifdef EVENT_OUTPUT
			printf("%s period=%lu freq=%lu\n", fds[i].name, (long unsigned int) fds[i].hw.sample_period, fds[i].hw.freq);
			#endif

			fds[i].hw.read_format = PERF_FORMAT_SCALE;
			if (num_fds[group] > 1)
				fds[i].hw.read_format |= PERF_FORMAT_GROUP|PERF_FORMAT_ID;
		}

		fds[i].hw.exclude_guest = 1;
		fds[i].hw.exclude_kernel = 1;

		fds[i].fd = perf_event_open(&fds[i].hw, -1, cpu, fds[0].fd, flags); // Profile every PID for a given CPU
		if (fds[i].fd == -1) {
			if (fds[i].hw.precise_ip)
				err(1, "cannot attach event %s: precise mode may not be supported", fds[i].name);
			err(1, "cannot attach event %s", fds[i].name);
		}
	}

	// kernel adds the header page to the size of the mmapped region
	fds[0].buf = mmap(NULL, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fds[0].fd, 0);
	if (fds[0].buf == MAP_FAILED)
		err(1, "cannot mmap buffer");

	// does not include header page
	fds[0].pgmsk = (mmap_pages*pgsz)-1;

	// send samples for all events to first event's buffer
	for (int i = 1; i < num_fds[group]; i++) {
		if (!fds[i].hw.sample_period)
			continue;
		ret = ioctl(fds[i].fd, PERF_EVENT_IOC_SET_OUTPUT, fds[0].fd);
		if (ret)
			err(1, "cannot redirect sampling output");
	}

	/*
	 * we are using PERF_FORMAT_GROUP, therefore the structure
	 * of val is as follows:
	 *
	 *      { u64           nr;
	 *        { u64         time_enabled; } && PERF_FORMAT_ENABLED
	 *        { u64         time_running; } && PERF_FORMAT_RUNNING
	 *        { u64         value;
	 *          { u64       id;           } && PERF_FORMAT_ID
	 *        }             cntr[nr];
	 *      }
	 * We are skipping the first 3 values (nr, time_enabled, time_running)
	 * and then for each event we get a pair of values.
	 */
	if (num_fds[group] > 1) {
		size_t sz = (3+2*num_fds[group])*sizeof(uint64_t);
		uint64_t val[3+2*num_fds[group]];

		ret = read(fds[0].fd, val, sz);
		if (ret == -1)
			err(1, "cannot read id %zu", sizeof(val));

		for(int i=0; i < num_fds[group]; i++) {
			fds[i].id = val[2*i+1+3];
			#ifdef EVENT_OUTPUT
			printf("%lu  %s\n", fds[i].id, fds[i].name);
			#endif

			#ifdef JUST_PROFILE
			if(cpu != 0)
				continue;

			char* e_name = strtok(fds[i].name, ":"); // To remove ":period=X" stuff
			if(e_name != NULL){
				if(strcmp("FP_COMP_OPS_EXE", e_name) == 0) // Incomplete FLOPS event name, let's get the next part
					e_name = strtok(NULL, ":");
				my_pebs_sample_t::add_subevent_name(e_name); // Dynamic column names
			}
			#endif
		}

		#ifdef JUST_PROFILE
		// We are assuming just a group of memory events (nr always 1) and another one of instructions (nr always > 1). Maybe it should be adapted for another kind of group events
		my_pebs_sample_t::max_nr = num_fds[group];
		#endif
	}
	return 0;
}

static void clean_end(int n) {
	perf_event_desc_t *fds = NULL;

	#ifdef EVENT_OUTPUT
	printf("TERMINATING\n");
	#endif

	// Closes and frees resources
	for(int i=0;i<NUM_GROUPS;i++){
		for(int j=0;j<system_struct_t::NUM_OF_CPUS;j++){
			fds = all_fds[i][j];
			for(int k=0; k < num_fds[i]; k++)
				close(fds[k].fd);

			munmap(fds[0].buf, map_size);
			perf_free_fds(fds, num_fds[i]);
		}
	}

	for(int i=0;i<NUM_GROUPS;i++)
		free(all_fds[i]);
	pfm_terminate();

	// Special print for a specific analysis: period,minlat,samples,meanacs
	//printf("%d,%d,%lu\n", options.periods[0], options.minimum_latency,processed_samples_group[0]);
	clean_migration_structures();

	#ifdef USE_ENERGY
	ed.close_buffers();
	#endif

	system_struct_t::clean();

	//#ifdef EVENT_OUTPUT
	const char* types[2] = {"memory", "instruction"};
	for(int i=0;i<NUM_GROUPS;i++)
		printf("%lu (%lu) %s samples collected (processed) in total %lu poll events and %lu lost samples\n", collected_samples_group[i],processed_samples_group[i], types[i], buffer_reads[i], lost_samples_group[i]);
	printf("%lu unknown samples.\n", unknown_samples);
	//#endif

	#if !defined(JUST_PROFILE) && defined(DO_MIGRATIONS)
	printf("%d thread migrations made.\n", migration_cell_t::total_thread_migrations);
	printf("%d page migrations made.\n", migration_cell_t::total_page_migrations);
	#endif

	exit(0);
}

int mainloop(char **arg) {
	// Obtains some important system "constants" in execution time
	int ret = system_struct_t::detect_system();
	if(ret != 0)
		exit(ret);

	#ifdef USE_ENERGY
	ret = ed.prepare_energy_data(options.base_filename); // Needs system_struct_t::detect_system() be called before
	if(ret != 0)
		exit(ret);
	#endif

	uid = getuid();

	#ifndef JUST_PROFILE
	tid_cpu_table.coln = system_struct_t::NUM_OF_CPUS; // Badly done because NUM_OF_CPUS is gotten in execution time
	#endif

	#ifdef FAKE_DATA
	work_with_fake_data();
	return 0;
	#endif

	const unsigned short TOTAL_BUFFS = system_struct_t::NUM_OF_CPUS*NUM_GROUPS;

	// This is the struct for polling the buffers of system_struct_t::NUM_OF_CPUS for different groups of events
	struct pollfd pollfds[TOTAL_BUFFS];
	int fd = -1;
	perf_event_desc_t *fds = NULL;

	// Initializes arrays to zero
	memset(collected_samples_group, 0, sizeof(collected_samples_group));
	memset(processed_samples_group, 0, sizeof(processed_samples_group));
	memset(buffer_reads, 0, sizeof(buffer_reads));
	memset(lost_samples_group, 0, sizeof(lost_samples_group));

	// Allocates memory for all_fds
	for(int i=0;i<NUM_GROUPS;i++){
		all_fds[i] = (perf_event_desc_t**)malloc(system_struct_t::NUM_OF_CPUS * sizeof(perf_event_desc_t *));
		if (!all_fds[i])
			err(1, "cannot allocate memory for all_fds[%d]",i);
	}

	// Initializes PFM
	if (pfm_initialize() != PFM_SUCCESS)
		errx(1, "libpfm initialization failed\n");

	// Sets up counter configuration
	for(int g=0;g<NUM_GROUPS;g++){
		for(int c=0;c<system_struct_t::NUM_OF_CPUS;c++)
			setup_cpu(c, fd, g);
	}

	// Sets up handler for some signals for a clean end
	signal(SIGALRM, clean_end);
	signal(SIGINT, clean_end);

	// This is for polling the buffers of system_struct_t::NUM_OF_CPUS cpus for the available groups
	for(int i=0;i<TOTAL_BUFFS;i++){
		int gr = i / system_struct_t::NUM_OF_CPUS;
		int cpu = i % system_struct_t::NUM_OF_CPUS;
		fds = all_fds[gr][cpu];
		pollfds[i].fd = fds[0].fd;
		pollfds[i].events = POLLIN;
	}

	// Starts counters
	for(int i=0;i<system_struct_t::NUM_OF_CPUS;i++){
		for(int j=0;j<NUM_GROUPS;j++){
			fds = all_fds[j][i];

			if (fds[0].fd == -1)
				continue;
			int ret = ioctl(fds[0].fd, PERF_EVENT_IOC_ENABLE, 0);
			if (ret)
				err(1, "cannot start counter");
		}
	}

	last_migr_time = time(NULL); // Initial time

	// Core loop where the polling to the buffers is done, has some issues
	for(;;) {
		#ifdef USE_ENERGY
		usleep(options.sbm * 1000); // Not the best way, but using polling may give issues to read energy buffers

		// Time is got and energy buffers are read
		time_t current_time = time(NULL);
		double secs = options.sbm * 1e-3;
		ed.read_buffer(secs);
		//ed.print_curr_vals(); // Just for testing we got the data
		//printf("Secs: %.3f. Pkg: %.3f\n", secs, ed.curr_vals[0][0]);
		#else
		// Core call under normal conditions: polling to counter buffers
		int ret = poll(pollfds, TOTAL_BUFFS, options.sbm);

		if (ret < 0 && errno == EINTR)
			break;
		#endif

		// Reads (only ready) buffers
		for(int g=0;g<NUM_GROUPS;g++){
			for(int c=0;c<system_struct_t::NUM_OF_CPUS;c++){
/*
				// Experimental
				#if ! (defined(JUST_PROFILE_ENERGY) || defined(USE_ENER_ST))
				int i =  g * system_struct_t::NUM_OF_CPUS + c; // Poll index
				if(pollfds[i].revents == 0) // No (full) new data
					continue;
				#endif
*/

				process_smpl_buf(all_fds[g][c], c, all_fds[g], num_fds[g], g);
				buffer_reads[g]++;
			}
		}

		#ifndef JUST_PROFILE
		perform_migration(); // We have the data, so we can begin the migration process
		#endif
	}

	return 0;
}

static void usage(void){
	printf("usage: my_profiler_tm [-h] [--help] [-b base_filename] [-p period_memory] [-P period_indtructions] [-l minimum_latency] [-s seconds_between_migrations]\n");
}

int main(int argc, char **argv){
	char c;

	// Defaults
	options.periods[0] = 1000;

	if(NUM_GROUPS > 1)
		options.periods[1] = 50000000;

	#ifdef USE_ENERGY
	options.base_filename = (char*)malloc(32*sizeof(char));
	strcpy(options.base_filename, "");
	#endif

	options.minimum_latency = 250;

	#ifdef USE_ENERGY
	options.sbm = 1000;
	#else
	options.sbm = -1;
	#endif

	static struct option the_options[]= {
		{ "help", 0, 0,  1},
		{0,0,0,0}
	};

	while ((c=getopt_long(argc, argv,"+h:b:p:P:l:s:", the_options, 0)) != -1) {
		switch(c) {
			case 'b': // Base_filename
				strcpy(options.base_filename, optarg);
				break;
			case 'p':
				// In the future it will be better if this gets redefined with something like "periodG0_periodG1_p..."
				options.periods[0] = atoi(optarg);
				break;
			case 'P':
				if(NUM_GROUPS > 1)
					options.periods[1] = atoi(optarg);
				break;
			case 'l':
				options.minimum_latency = atoi(optarg);
				break;
			case 's':
				options.sbm = atoi(optarg);
				break;
			case 'h':
				usage();
				exit(0);
			default:
				errx(1, "unknown option");
		}
	}

	return mainloop(argv+optind);
}
