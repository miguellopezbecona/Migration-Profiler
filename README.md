## Overview
This application is a profiler which potentially will use thread and page migration in order to improve efficiency in multi-threaded applications, specially in NUMA systems. It uses PEBS sampling to obtain information from hardware counters so it can analyze system's performance and make choices to improve it. It is heavily based on the work done by Óscar García Lorenzo for his PhD.

The repository includes a test application to be profilled called `ABC`. It performs a simple element by element vector product which can be very customizable with options such as array size, CPUs to be used, number of repetitions, intensity of each iteration, etc.

The app can also print, each N iterations, some CSV files that can be plotted a posteriori using *heatmaps.R* file to analyze the system's "state". It depends if the `PRINT_CSVS` macro is defined, currently commented in `page_ops.h` file. Nowadays, 5 files are generated:

* Four heatmaps where X axis is the number of memory page, Y axis is the number of thread, and the value itself is the mean/maximum/mininum/number of latency accesses depending of the file.
* A line plot where X axis is the number of memory page and Y axis is the number of different threads that access to that memory page.

## Compiling and executing
The app requires `libnuma-dev` package, so you need to install it first. It also uses [libpfm](http://perfmon2.sourceforge.net/) but it is already compiled and included in the repository. Note that `perf_event_paranoid` system file should contain a zero or else the profiler will not be able to read the hardware counters. You can solve it with the following command:
```bash
echo "0" | sudo tee /proc/sys/kernel/perf_event_paranoid > /dev/null
```

If you just want to build the profiler, you can use the Makefile inside the source code folder. I have already facilitated a bash script to also execute it along the ABC program. Just go to the source code folder and execute "test" script:
```bash
bash test.sh [NUMA option]
```

Apart from building the profiler, it will execute it along the ABC program with some static parameters. You can edit the script freely so it profiles another application. Note that right now, it can only profile a child process created by the own profiler and not a previous running process. Furthermore, the test script allows you to indicate a NUMA configuration:

* *d* (direct): uses all CPUs from memory node 0 and allocates data in memory node 0. This is considered a "good" case that should not require any migrations.
* *ind* (indirect): uses all CPUs from memory node 1 and allocates data in memory node 0.  This is considered a "bad" case that should require some migrations.
* *int* (interleave): uses interleave option from `numactl` command.

## Execution flow, components, etc
Note that some files are leftovers from Óscar's work. The following list explains briefly the components regarding the main execution flow:

* `my_profiler.c`: all begins in this file. It is based on libfpm's example file `syst_smpl.c`. It sets up the profiler, creates the child process to be profiled, gets sample data into a general structure (`my_pebs_sample_t`) and then it is inserted in one of two lists (`memory_list` and `inst_list`, currently in `thread_migration` folder). Then, `perform_migration` is called, which calls `do_migration_and_clear_temp_list`.
  - `migration/memory_list.c`: each element of the list consists in a structure that holds data for memory samples. It contains data such as memory address, latency, source CPU, source PID, etc.
  - `migration/inst_list.c`: each element of the list consists in a structure that holds data for instructions samples. It contains data such as instruction number, source CPU, source PID, etc.
* `igration/thread_migration.c`: serves as a fachade to other migration functionalities and holds the main data structures (`memory_data_list`, `inst_data_list`, and `main_page_table`). `do_migration_and_clear_temp_list` calls `do_migration` and at the end of the iteration, the two lists mentioned before are emptied. `do_migration` creates increments (instruction count) for the inst_list, builds the page table calling `pages` and performs a migration strategy. Might be renamed.
* `migration/page_ops.c`: serves as a fachade to other page migration functionalities. `pages` creates page table and creates and closes the CSV files to be generated. Might be discarded.
* `migration/page_table.c`: it consists in an somewhat "static" array of maps (i.e, a matrix with a fixed number of rows). The rows would be the number of threads, while the columns would be the memory pages' numbers. So, for each access a thread does to a memory page, this structure holds a cell with latencies and cache misses. This will be the main data structure to be used in future work to compute system's performance and migration decisions.
* `migration/page_migration_algorithm.c`: it calls freely the strategies you want, defined in `strategies` folder in order to do the following migrations.
* `migration/migration_cell.*`: defines a migration, which needs an element to migrate (TID or memory page) and a destination (core or memory node).
* `strategies/strategy.h`: defines which operations should define a scratch strategy.
* `strategies/random.*`: right now, the only defined strategy is a simple random approach.

The following is the brief explanation of some of the other files:
* `perfmon/*`: for `libpfm` library. Should not be modified. It also includes `perf_util.*` which holds the data obtained by the counters.
* `thread_migration/system_struct.*`: defines system's structure (number of cores, number of memory nodes, distribution of CPUs/threads on memory nodes...) and functions to get this data. It is intended to detect all these parameters in a dynamic way.

## Design [TODO]
A UML diagram will be generated to make clearer the connection between the components.

## License
Private, right now.

