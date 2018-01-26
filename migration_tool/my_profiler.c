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

// Using a value greater than 2 requires additional changes in, at least, "events" and "periods" array
#define NUM_GROUPS 2

// Uncomment the following for testing functionalities without using the hardware counters
//#define FAKE_DATA

//#define EVENT_OUTPUT

typedef struct {
	int mmap_pages;
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
static size_t pgsz;
static size_t map_size;

time_t last_migr_time;

static const char *events[2] = {
	//"RAPL_ENERGY_PKG:period=1000",
	"MEM_TRANS_RETIRED:LATENCY_ABOVE_THRESHOLD:period=1000"
	,"INST_RETIRED:period=1000000,OFFCORE_REQUESTS:ALL_DATA_RD"
};

static void clean_end(int n);

#ifndef JUST_PROFILE
void perform_migration(){
	time_t current_time = time(NULL);

	// We profile every 1, 2 or 4 seconds depending on current_time_value
	if((difftime(current_time,last_migr_time)) > (get_time_value() * inv_1000)){
		last_migr_time = current_time;
		//printf("\n***********\nAt %s\n",ctime(&last_migr_time));

		begin_migration_process();
	}
}
#endif

static void process_smpl_buf(perf_event_desc_t *hw, int cpu, perf_event_desc_t **all_fds_p, int num_fds_p, int name) {
	my_pebs_sample_t my_sample;
	struct perf_event_header ehdr;
	perf_event_desc_t *fds = NULL;

	fds = all_fds_p[cpu];
	my_sample.values = (uint64_t*)malloc(sizeof(uint64_t)*num_fds_p);	

	for(;;) {
		int ret = perf_read_buffer(hw, &ehdr, sizeof(ehdr));
		if (ret)
			return; /* nothing to read */

		switch(ehdr.type) {
			case PERF_RECORD_SAMPLE:
				ret = transfer_data_from_buffer_to_structure(fds, num_fds_p, hw - fds, &ehdr, &my_sample, uid);
				processed_samples_group[name]++;

				if(ret < 0) // PID from process we can't migrate so it will be skipped
					break;

				add_data_to_list(my_sample);
				
				if (ret)
					errx(1, "cannot parse sample");

				collected_samples_group[name]++;
				break;
			case PERF_RECORD_EXIT:
				//display_exit(hw, options.output_file);
				break;
			case PERF_RECORD_LOST:
				lost_samples_group[name] += display_lost(hw, fds, num_fds_p, NULL);
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

				// Let's assume we won't get trapped in a infinite loop
				/*
				if(unknown_samples > 50){
					printf("Error while reading buffer, too many unknown samples, exiting...\n");
					clean_end(-1);
					exit(-1);
				}
				*/

				break;
		}
		
	}
}

int setup_cpu(int cpu, int fd, int group) {
	perf_event_desc_t *fds = NULL;
	int ret, flags;

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
				fds[i].hw.wakeup_watermark = (options.mmap_pages*pgsz) / 2;
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
		fds[i].hw.exclude_kernel = 1; // Let's exlcude kernel

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
	fds[0].pgmsk = (options.mmap_pages*pgsz)-1;

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
		}
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

	system_struct_t::clean();

	//#ifdef EVENT_OUTPUT
	const char* types[2] = {"memory", "instruction"};
	for(int i=0;i<NUM_GROUPS;i++)
		printf("%lu (%lu) %s samples collected (processed) in total %lu poll events and %lu lost samples\n", collected_samples_group[i],processed_samples_group[i], types[i], buffer_reads[i], lost_samples_group[i]);
	printf("%lu unknown samples.\n", unknown_samples);
	//#endif

	#if ! defined(JUST_PROFILE) //&& defined(EVENT_OUTPUT)
	printf("%d thread migrations made.\n", migration_cell_t::total_thread_migrations);
	printf("%d page migrations made.\n", migration_cell_t::total_page_migrations);
	#endif

	exit(0);
}

int mainloop(char **arg) {
	// Obtains some important "constants" in execution time
	system_struct_t::detect_system();

	uid = getuid();
	pgsz = sysconf(_SC_PAGESIZE);
	map_size = (options.mmap_pages+1)*pgsz;

	tid_cpu_table.coln = system_struct_t::NUM_OF_CPUS; // Badly done because NUM_OF_CPUS is gotten in execution time

	#ifdef FAKE_DATA
	work_with_fake_data();
	return 0;
	#endif

	const unsigned short TOTAL_BUFFS = system_struct_t::NUM_OF_CPUS*NUM_GROUPS;
	last_migr_time = time(NULL);

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
	for(int i=0;i<NUM_GROUPS;i++){
		for(int j=0;j<system_struct_t::NUM_OF_CPUS;j++)
			setup_cpu(j, fd, i);
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

	// Core loop where the polling to the buffers is done, has some issues
	for(;;) {

		int ret = poll(pollfds, TOTAL_BUFFS, options.sbm);
		if (ret < 0 && errno == EINTR)
			break;

		// Reads buffers
		for(int i=0;i<NUM_GROUPS;i++){
			for(int j=0;j<system_struct_t::NUM_OF_CPUS;j++){
				process_smpl_buf(all_fds[i][j], j, all_fds[i], num_fds[i], i);
				buffer_reads[i]++;
			}
		}

		#ifndef JUST_PROFILE
		perform_migration(); // We have the data, so we can begin the migration process
		#endif
	}

	return 0;
}

static void usage(void){
	printf("usage: my_profiler_tm [-h] [--help] [-p period_memory] [-P period_indtructions] [-l minimum_latency] [-s seconds_between_migrations]\n");
}

int main(int argc, char **argv){
	char c;

	// Defaults
	options.periods[0] = 750;

	if(NUM_GROUPS > 1)
		options.periods[1] = 10000000;

	options.minimum_latency = 250;
	options.sbm = -1; // Infinite timeout by default
	options.mmap_pages = 16;

	static struct option the_options[]= {
		{ "help", 0, 0,  1},
		{0,0,0,0}
	};

	while ((c=getopt_long(argc, argv,"+h:p:P:l:s:", the_options, 0)) != -1) {
		switch(c) {
			case 0: continue;
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

	if (options.mmap_pages > 1 && ((options.mmap_pages) & 0x1))
		errx(1, "number of pages must be power of 2\n");
	
	return mainloop(argv+optind); 
}
