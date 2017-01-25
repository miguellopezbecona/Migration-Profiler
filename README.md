## Overview
This application is a profiler which potentially will use thread and page migration in order to improve efficiency in multi-threaded applications, specially in NUMA systems. It uses PEBS sampling to obtain information from hardware counters so it can analyze system's performance and make choices to improve it. It is heavily based on the work done by Óscar García Lorenzo for his PhD.

The repository includes a test application to be profilled called `ABC`. It performs a simple element by element vector product which can be very customizable with options such as array size, CPUs to be used, number of repetitions, intensity of each iteration, etc.

The app can also print, each N iterations, some CSV files that can be plotted a posteriori using *heatmaps.R* file to analyze the system's "state". Nowadays, 5 files are generated:

* Four heatmaps where X axis is the number of memory page, Y axis is the number of thread, and the value itself is the mean/maximum/mininum/number of latency accesses depending of the file.
* A line plot where X axis is the number of memory page and Y axis is the number of different threads that access to that memory page.

## Compiling and executing
The app requires `libnuma-dev` package, so you need to install it first. It also uses [libpfm](http://perfmon2.sourceforge.net/) but it is already compiled and included in the repository. Note that `perf_event_paranoid` system file should contain a zero or else the profiler will not be able to read the hardware counters. You can solve it with the following command:
```bash
echo "0" | sudo tee /proc/sys/kernel/perf_event_paranoid > /dev/null
```

For compiling and executing the profiler, I have already facilitated a bash script to do it. Just go to the source code folder and execute "test" script:
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
  - `thread_migration/memory_list.c`: each element of the list consists in a structure that holds data for memory samples. It contains data such as memory address, latency, source CPU, source PID, etc.
  - `thread_migration/inst_list.c`: each element of the list consists in a structure that holds data for instructions samples. It contains data such as instruction number, source CPU, source PID, etc.
* `thread_migration/thread_migration.c`: serves as a fachade to other migration functionalities and holds the main data structures (`memory_data_list`, `inst_data_list`, `pid_map` and `main_page_table`). `do_migration_and_clear_temp_list` calls `do_migration` and at the end of the iteration, the two lists mentioned before are emptied. `do_migration` creates increments (instruction count) for the inst_list, calls `rooflines` data and builds the page table.
* `thread_migration/rooflines_ops.c`: builds a temporal list, a PID map (key: PID, value: list of TIDs for that PID) and calculates thread performance basing on processed instructions and memory latency. May be deprecated in future work.
  - `thread_migration/temp_tid_list.c`: mixes memory and instruction data to calculate performance. Could be overrided in the future.
  - `thread_migration/pid_list.c`: includes a structure that holds the PID map described above and a variable of `tid_map` type.
  - `thread_migration/tid_list.c`: includes a structure that holds a TID map (key: TID, value: `tid_cell_t`). It also defines `tid_cell_t` structure, which can hold data such as assigned core, performance, etc.
  - Previously, `pid_list` was implemented as a list so each element held a list of TIDs, but it raised memory addressing issues in pointers.
* `page_migration/page_ops.c`: serves as a fachade to other page migration functionalities. `pages` creates page table and creates and closes the CSV files to be generated.
* `page_migration/page_table.c`: it consists in an static array of maps (i.e, a matrix with a fixed number of rows). The rows would be the number of threads, while the columns would be the memory pages' numbers. So, for each access a thread does to a memory page, this structure holds a cell with latencies and cache misses. This will be the main data structure to be used in future work to compute system's performance and migration decisions.
* `page_migration/page_migration_algorithm.c`: it is currently inoperative, but it would be the module that computes data from the structure described above and then perform migrations with that data.
* After the page table is created, the iteration is over because there is no migration strategy for that data yet. The code in `thread_migration.c` can be altered so it uses Óscar's thread-only migration strategy instead.

The following is the brief explanation of some of the other files:
* `perfmon/*`: for `libpfm` library. Should not be modified.
* `thread_migration/system_struct.*`: defines system's structure (number of cores, number of memory nodes, distribution of CPUs/threads on memory nodes...) and functions to get this data. It is intended to detect all the parameters in a dynamic way, but currently the number of cores and memory nodes are statically written due to the use of these values in static arrays. It will be arranged in the future.
* `thread_migration/thread_migration_aux.*`: defines auxiliar functions to migrate threads. Leftover from Óscar's work that may be reused.
* `thread_migration/migration_*.*`: some other files that aid the thread-only migration strategy.
* `page_migration/tid_access_list.c`: could be added to each cell in `page_table`, but it is currently a leftover that may be deleted.

## Design [TODO]
A UML diagram will be generated to make clearer the connection between the components.

## License
Private, right now.

