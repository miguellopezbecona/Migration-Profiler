#!/bin/bash

#make my_profiler_thread_migration_opt

execname=my_profiler_tm
profparams="-s2000 -l400 -p5000 -P50000000"
#profparams="-s3500 -l750 -p1000 -P7500000"
#profparams="-s1000 -l500 -p5000 -P1000000"

gcc -fopenmp -o ABC ABC.c -lnuma

# Uses numactl (and in different ways) or not depending on first parameter
if [ $# -lt 1 ]; then
	numacommand=
elif [ $1 == "d" ]; then
	numacommand="numactl --cpubind=0 --membind=0"
elif [ $1 == "ind" ]; then
	numacommand="numactl --cpubind=1 --membind=0"
elif [ $1 == "int" ]; then
	numacommand="numactl --interleave=0,1"
else
	numacommand=
fi

# Set to what you want to profile
toprofile="./ABC -m0 -r5000 -s1000000 -c1111 -t8"
#toprofile=~/NPB3.3.1/NPB3.3-OMP/bin/lu.C.x
#toprofile=~/NPB3.3.1/NPB3.3-OMP/bin/bt.C.x

# Compiles and executes
g++ -o $execname [^ABC]*.c perfmon/*.c thread_migration/*.c page_migration/*.c libpfm.so.4.8.0 -lnuma && $numacommand ./$execname -M $profparams $toprofile

# Does not continue if there was an error
if [ $? -ne 0 ]; then
	exit 1
fi

### Preprocessing of generated files

# Removes last comma from each CSV
sed -i 's/.$//' acs_*.csv max_*.csv min_*.csv avg_*.csv

# Latest step index of generated files
latest=$(ls -v acs_*.csv | tail -n 1 | sed -r "s/acs_([0-9]+).csv/\1/g")

# Deletes non-final CSVs (it is not necessary)
find . -maxdepth 1 ! -name "*_$latest.csv" | grep .csv | xargs rm

# Renames final CSVs
rename "s/_$latest//g" *.csv # This works in local
#rename "_$latest" "" *.csv # This works in server

