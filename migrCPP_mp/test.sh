#!/bin/bash

#make my_profiler_thread_migration_opt
execname=my_profiler_tm

gcc -fopenmp -o ABC ABC.c -lnuma

# Basic test
#g++ -o $execname [^ABC]*.c perfmon/*.c thread_migration/*.c page_migration/*.c libpfm.so.4.7.0 -lnuma && time ./$execname -m -s1000 -l500 -p5000 -P1000000 ./ABC -m1 -r5000 -s1000000 -c1111 -t8
#g++ -o $execname [^ABC]*.c perfmon/*.c thread_migration/*.c page_migration/*.c libpfm.so.4.7.0 -lnuma && ./$execname -M -s2000 -l400 -p5000 -P50000000 ./ABC -m0 -r5000 -s1000000 -c1111 -t8
g++ -Wno-format -o $execname [^ABC]*.c perfmon/*.c thread_migration/*.c page_migration/*.c libpfm.so.4.7.0 -lnuma #&& ./$execname -M -s2000 -l400 -p5000 -P50000000 ./ABC -m0 -r5000 -s1000000 -c1111 -t8

# For benchmark (thread migration)
#g++ -o $execname [^ABC]*.c perfmon/*.c thread_migration/*.c page_migration/*.c libpfm.so.4.7.0 -lnuma && ./$execname -m -s2000 -l400 -p500 -P50000000 ~/NPB3.3.1/NPB3.3-OMP/bin/lu.C.x ; pkill -9 lu.C.x

# For benchmark (page migration)
#g++ -o $execname [^ABC]*.c perfmon/*.c thread_migration/*.c page_migration/*.c libpfm.so.4.7.0 -lnuma && ./$execname -M -s3500 -l750 -p1000 -P7500000 ~/NPB3.3.1/NPB3.3-OMP/bin/bt.C.x ; pkill -9 bt.C.x

#sed -i 's/.$//' acs_*.csv max_*.csv min_*.csv avg_*.csv
