## Overview
This folder contains two applications:
* `rapl.cpp`: a C++ profiler that uses Running Average Power Limit (RAPL) to read energy consumptions for all the nodes and RAPL domains from the system. It has two modes: `ONE_MEASURE` and a incremental profiling, depending in the mentioned macro is whether commented or not. The first mode, as the name suggest, opens the counters and only reads its values when the profiler ends, so metrics like total consumption or mean consumption per second (for each node, domain, and in total). The other mode reads the counters periodically (this value can be customized with the `-p` parameter), so it can print the information or just dump the data into CSV files to be analyzed a posteriori with the `rapl.R` file provided (still in construction). The C++ app will probably be merged with the main migration thread/page migration tool. In fact it uses a copy of its `system_struct_t` component.
* `my_test.c`: a test application, heavily based on the `ABC` test program in `migration_tool` folder, that intends to help understanding energy consumption in a system by parametrizing specific operations such as read operations, node remote reads, floating point operations, etc. It uses a simplified version of the `system_struct_t` component from the main migration tool to be C-compliant. Its parameters are the following:
  - `-l`: read iterations. It reads consecutive float elements from a local array (default: 1000).
  - `-d`: elements read per iteration. It reads elements from different cache lines from a local array (default: 1).
  - `-r`: remote elements read per iteration. It reads elements from different cache lines from a remote array (default: 0).
  - `-o`: floating point operations per iteration (default: 1).
  - `-t`: number of threads, which would work in different chunks of the arrays. Will be set to `CPUS_PER_MEMORY` if an out of bound value is provided, because threads will be pinned to CPUs from the same (local) node (default: 1).
  - `-m`: local node reference. Will be set to 0 if an out of bound value is provided (default: 0).
  - `-M`: remote node reference. Will be set to `NUM_OF_MEMORIES-1` if an out of bound value is provided (default: 1).
  - The total array size depends on `-l` and `-t` parameters.

Some macros can be commented/uncommented in both applications to increase/decrease the amount of printing, they all have a `OUTPUT`-like name.

## Compiling and executing
Using RAPL requires a Linux system with a 3.13+ kernel version. RAPL values are read using a `perf_event_open` fashion so, unless you are root, `perf_event_paranoid` system file should contain a zero or else the profiler will not be able to read the hardware counters. You can solve it with the following command:
```bash
echo "0" | sudo tee /proc/sys/kernel/perf_event_paranoid > /dev/null
```

The C app requires `libnuma-dev` package, so you need to install it first.

If you just want to build the profiler, you can use the Makefile inside the source code folder. I have already facilitated a bash script to also execute it along with the test program. Just go to the source code folder and execute `test` script:
```bash
bash test.sh
```

In normal conditions, the profiler never ends and needs a `SIGINT` (Ctrl+C) signal to end in a clean way. 

## License
Do whatever you want with this code.

