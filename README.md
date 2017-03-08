## Overview
This application is a profiler which potentially will use thread and page migration in order to improve efficiency in multi-threaded applications, specially in NUMA systems. It uses PEBS sampling to obtain information from hardware counters so it can analyze system's performance and make choices to improve it. It is heavily based on the work done by Óscar García Lorenzo for his PhD.

The repository includes a test application to be profilled called `ABC`. It performs a simple element by element vector product which can be very customizable with options such as array size, CPUs to be used, number of repetitions, intensity of each iteration, etc.

The app can also print, each N iterations, some CSV files that can be plotted a posteriori using *heatmaps.R* file to analyze the system's "state". It depends if the `PRINT_CSVS` macro is defined, currently commented in `page_ops.h` file. Nowadays, 5 files are generated:

* Four heatmaps where X axis is the number of memory page, Y axis is the number of thread, and the value itself is the mean/maximum/mininum/number of latency accesses depending of the file.
* A line plot where X axis is the number of memory page and Y axis is the number of different threads that access to that memory page.

## Compiling and executing
The app requires `libnuma-dev` package, so you need to install it first. It also uses [libpfm](http://perfmon2.sourceforge.net/) but it is already compiled and included in the repository. Note that, unless you are root, `perf_event_paranoid` system file should contain a zero or else the profiler will not be able to read the hardware counters. You can solve it with the following command:
```bash
echo "0" | sudo tee /proc/sys/kernel/perf_event_paranoid > /dev/null
```

If you just want to build the profiler, you can use the Makefile inside the source code folder. I have already facilitated a bash script to also execute it along the ABC program. Just go to the source code folder and execute "test" script:
```bash
bash test.sh [NUMA option]
```

Apart from building the profiler, it will execute it along the ABC program with some static parameters. You can edit the script freely so it profiles another application. Furthermore, the test script allows you to indicate a NUMA configuration:

* *d* (direct): uses all CPUs from memory node 0 and allocates data in memory node 0. This is considered a "good" case that should not require any migrations.
* *ind* (indirect): uses all CPUs from memory node 1 and allocates data in memory node 0.  This is considered a "bad" case that should require some migrations.
* *int* (interleave): uses interleave option from `numactl` command.

## Execution flow, components, etc
The following list explains briefly the components regarding the main execution flow:

* `my_profiler.c`: all begins in this file. It is based on libfpm's example file `syst_smpl.c`. It sets up the profiler and gets sampling data into a general structure (`my_pebs_sample_t`), filtering undesired PIDs (because we might not have permissions) and then inserting it in the correct list (`memory_list` or `inst_list`). Then, `perform_migration` is called, which calls `begin_migration_process`.
  - `migration/memory_list.c`: each element of the list consists in a structure that holds data for memory samples. It contains data such as memory address, latency, source CPU, source PID, etc.
  - `migration/inst_list.c`: each element of the list consists in a structure that holds data for instructions samples. It contains data such as instruction number, source CPU, source PID, etc.
* `migration/migration_facade.c`: serves as a facade to other migration functionalities and holds the main data structures (`memory_data_list`, `inst_data_list`, and `page_tables`). `begin_migration_process` creates increments (instruction count) for `inst_data_list` that might be used in the future, builds a page table for each wanted PID to profile by calling `pages`, and performs a migration strategy. At the end, `memory_data_list` and `inst_data_list` are emptied.
* `migration/page_ops.c`: builds page tables (defined in the following file) and can do other stuff like generating the already described CSV files. Might be fused with `migration_facade`.
* `migration/page_table.c`: it consists in a matrix defined as a vector of maps, where the rows (number of vectors) would be the number of TIDs, while the columns would be the memory pages' addresses. So, for each access a thread does to a memory page, this structure holds a cell with latencies and cache misses. Another option was implement it as a single map with a pair of two values as key, but that would make inefficient the looping over a specific row (TID) or a specific column (page address). TID indexing is done by a simple "bad hash" using a counter rather than the actual TID, so each row isn't a map as well. This structure will be the main one to be used in future work to compute system's performance and migration decisions.
* `migration/migration_algorithm.c`: it calls freely the strategies you want, defined in `strategies` folder, in order to do the following migrations.
* `migration/migration_cell.*`: defines a migration, which needs an element to migrate (TID or memory page) and a destination (core or memory node) and functions to perform them.
* `strategies/strategy.h`: defines which operations should define a scratch strategy.
* `strategies/random.*`: right now, the only defined strategy is a simple random approach.

The following is the brief explanation of some of the other files:
* `perfmon/*`: most of its content comes from `libpfm` library, so in general it should not be modified. `perf_util.c` may be interesting because it defines how to get the counter data into the structure defined in `sample_data`.
* `sample_data.*`: defines a simple structure that holds the data obtained by the counters.
* `utils.*`: self descriptive.
* `migration/system_struct.*`: defines system's structure (number of cores, number of memory nodes, distribution of CPUs/threads on memory nodes...) and functions to get this data. It is intended to detect all these parameters in a dynamic way.

## Design [TODO]
A UML diagram will be generated to make clearer the connection between the components.

## License
Private, right now.

