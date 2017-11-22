## Overview
This application is a profiler which potentially will use thread and page migration in order to improve efficiency in multi-threaded applications, specially in NUMA systems. It uses PEBS sampling to obtain information from hardware counters so it can analyze system's performance and make choices to improve it. It is heavily based on the work done by Óscar García Lorenzo for his PhD.

The repository includes a test multithreaded application to be profiled called `ABC`. It performs a simple element by element vector product which can be very customizable with options such as array size, CPUs to be used, number of repetitions, intensity of each iteration, etc.

The app includes a simple mode where it just dumps the hardware counter data to CSV files, and the process hierarchy into JSON files. To activate it, you have to uncomment the `JUST_PROFILE` macro defined in `migration/migration_facade.h`. Each pair of files will be generated each `ELEMS_PER_PRINT` (defined in the same header) general samples (mixing instructions and memory ones). Other macros can be commented/uncommented to increase/decrease the amount of printing, such as `EVENT_OUTPUT` in `my_profiler.c`, `ANNEALING_OUTPUT` in `strategies/annealing.h`, `GENETIC_OUTPUT` in `strategies/genetic.h`, `PERFORMANCE_OUTPUT` in `strategies/page_ops.h` or `TH_MIGR_OUTPUT` and `PG_MIGR_OUTPUT` in `migration/migration_algorithm.h`.

Aside from that mode, the app can also print, each `ITERATIONS_PER_PRINT` (defined in `migration/page_ops.h`) iterations, some additional CSV files that can be plotted a posteriori using *heatmaps.R* file to analyze the system's "state". It depends if the `PRINT_CSVS` macro is defined, currently commented in the previously mentioned header file. Currently, 5 different CSV are generated each time:

* Four heatmaps where X axis is the number of memory page, Y axis is the number of thread, and the value itself is the mean/maximum/mininum/number of latency accesses depending of the file.
* A line plot where X axis is the number of memory page and Y axis is the number of different threads that access to that memory page.

## Compiling and executing
The app requires `libnuma-dev` package, so you need to install it first. It also uses [libpfm](http://perfmon2.sourceforge.net/) but it is already compiled (for a Linux *x86_64* architecture) and included in the repository. Note that, unless you are root, `perf_event_paranoid` system file should contain a zero or else the profiler will not be able to read the hardware counters. You can solve it with the following command:
```bash
echo "0" | sudo tee /proc/sys/kernel/perf_event_paranoid > /dev/null
```

Some Linux systems may also require having the enviroment variable `LD_LIBRARY_PATH` defined with a path that includes the directory where the app is, being the simplest way to solve it:
```bash
export LD_LIBRARY_PATH=.
```

If you just want to build the profiler, you can use the Makefile inside the source code folder. I have already facilitated two bash scripts, one to also execute it along with the ABC program and another one to execute the profiler until you kill it, because in normal conditions it never ends and needs a `SIGINT` (Ctrl+C) signal to end in a clean way. Just go to the source code folder and execute "test" or "test_forever" script:
```bash
bash test.sh [NUMA option]
```

Both the ABC and the profiler use some static parameters described below. You can obviously edit the script freely to change them. Now the ABC and profiler parameters will be described:

* `ABC`
  - `-b`: how many consecutive chunks will every thread work in (default: 1).
  - `-c`: binary vector of CPUs to be used (0 -> not used, 1 -> used). For example, 0011 means we only select CPUs 2 and 3 (default: uses all available CPUs). It is recommended to use `numactl --physcpubind cores` instead if your system has many cores and you want to select just a few.
  - `-m`: memory node to allocate data in (default: 0).
  - `-o`: number of float operations per inner iteration (default: 1).
  - `-r`: number of repetitions of the main flow (default: 1000).
  - `-s`: array total size. Each thread will work in same-size chunks, so this value can be altered to distribute the work problem perfectly (default: 1000000).
  - `-t`: array stride while operating (default: 1).
* `my_profiler`
  - `-G`: whether uses cgroups or not (default: not). It will probably be discarded.
  - `-l`: minimum memory latency access to sample (default: 200).
  - `-p`: period for memory samples (default: 1000). It will probably be rewritten to include all event groups' periods.
  - `-P`: period for instruction samples (default: 10000000). It will probably be discarded.
  - `-s`: polling timeout in milliseconds (default: -1, which implies no timeout). Using default value seems to give issues. 1000 is a good value.

Furthermore, the test script allows you to indicate a NUMA configuration:

* *d* (direct): uses all CPUs from memory node 0 and allocates data in memory node 0. This is considered a "good" case that should not require any migrations.
* *ind* (indirect): uses all CPUs from memory node 1 and allocates data in memory node 0.  This is considered a "bad" case that should require some migrations.
* *int* (interleave): uses interleave option from `numactl` command.

## Execution flow, components, etc
The following list explains briefly the components regarding the main execution flow:

* `my_profiler.c`: all begins in this file. It is based on libfpm's example file `syst_smpl.c`. It sets up the profiler and gets sampling data into a general structure (`my_pebs_sample_t`), filtering undesired PIDs (because we might not have permissions) and then inserting it in the correct list (`memory_list` or `inst_list`). Then, `perform_migration` is called, which calls `begin_migration_process`.
  - `migration/memory_list`: each element of the list consists in a structure that holds data for memory samples. It contains data such as memory address, latency, source CPU, source PID, etc.
  - `migration/inst_list`: each element of the list consists in a structure that holds data for instructions samples. It contains data such as instruction number, source CPU, source PID, etc.
* `migration/migration_facade`: serves as a facade to other migration functionalities and holds the main data structures (`memory_data_list`, `inst_data_list`, and `page_tables`). `begin_migration_process` creates increments (instruction count) for `inst_data_list` that might be used in the future, builds a page table for each wanted PID to profile by calling `pages`, and performs a migration strategy. At the end, `memory_data_list` and `inst_data_list` are emptied.
* `migration/page_ops`: builds page tables (defined in the following file) and can do other stuff like generating the already described CSV files. Might be fused with `migration_facade`.
* `migration/page_table`: it consists in a matrix defined as a vector of maps, where the rows (number of vectors) would be the number of TIDs, while the columns would be the memory pages' addresses. Each "profilable" process will be associated with one table. So, for each access a thread does to a memory page, this structure holds a cell with latencies, cache misses and another potential relevant information. Here is an example:
![Example page table](https://raw.githubusercontent.com/miguellopezbecona/Migration-Profiler/master/img/example_table.png)
Another option was implementing it as a single map with a pair of two values as key, but that would make inefficient the looping over a specific row (TID) or a specific column (page address). TID indexing is done by a simple "bad hash" using a counter rather than the actual TID, so we don't need two level of maps. Each table also holds other useful information such as maps to know in which memory node each page address is located or performance data. This structure will be the main one to be used in future work to compute system's performance and migration decisions.
* `migration/performance`: contains structures and functions for different performance values. Currently it has two, one defined in Óscar's work and the scratch of a new one.
* `migration/migration_algorithm`: it calls freely the strategies you want, defined in `strategies` folder, in order to do the following migrations.
* `migration/migration_cell`: defines a migration, which needs an element to migrate (TID or memory page) and a destination (core or memory node) and functions to perform them. A PID field and previos locations were also added to ease some operations, and a boolean that indicates wheter it is a thread or a memory cell.
* `strategies/strategy.h`: defines which operations should define a scratch strategy.
* `strategies/random`: there is a simple strategy implemented which is a simple random approach.
* `strategies/annealing`: this strategy is based on what Óscar did in his PhD work.
* `strategies/genetic` and `strategies/genetic/*`: I am working in a strategy based on a genetic algorithm, but in a simplified way.
* `strategies/first_touch`: simple strategy for memory pages which migrates the ones accessed by CPUs from different nodes.

The following is the brief explanation of some of the other files:
* `perfmon/*`: most of its content comes from `libpfm` library, so in general it should not be modified. `perf_util.c` may be interesting because it defines how to get the counter data into the structure defined in `sample_data`.
* `sample_data`: defines a simple structure that holds the data obtained by the counters. It can be printed into a CSV file.
* `utils`: self descriptive. Implements functions regarding the collection of information from the Linux system, timing and some printing.
* `migration/system_struct`: defines system's structure (number of CPUs, number of memory nodes, distribution of CPUs/threads on memory nodes...) and functions to get and set this data. The first mentioned static values are detected automatically, with no input needed.

## Design [TODO]
A UML diagram will be generated to make clearer the connection between the components.

## License
Private, right now.

