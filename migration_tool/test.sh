#!/bin/bash

execname=my_profiler_tm
profparams="-s2000 -l400 -p5000 -P50000000"
#profparams="-s3500 -l750 -p1000 -P7500000"
#profparams="-s1000 -l500 -p5000 -P1000000"

# Goes to source code folder if you execute script from another directory
dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $dir

# Compiles ABC
gcc -fopenmp -o ABC ABC.c -lnuma

# Compiles profiler
make

# Exits if it there was a problem
if [ $? -ne 0 ]; then
	exit 1
fi

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
toprofile="./ABC -b1 -m0 -r5000 -s1000000 -t8"
#toprofile=~/NPB3.3.1/NPB3.3-OMP/bin/lu.C.x
#toprofile=~/NPB3.3.1/NPB3.3-OMP/bin/bt.C.x

# Executes profiler with the app to profile as a child process
$numacommand ./$execname -M $profparams $toprofile

# Does not continue if there was an error
if [ $? -ne 0 ]; then
	exit 1
fi

# To skip file processing right now
exit 0

### Preprocessing of generated files

# Removes last comma from each CSV
sed -i 's/.$//' acs_*.csv max_*.csv min_*.csv avg_*.csv

# Latest step index of generated files
latest=$(ls -v acs_*.csv | tail -n 1 | sed -r "s/acs_([0-9]+).csv/\1/g")

# Deletes non-final CSVs (it is not necessary)
find . -maxdepth 1 ! -name "*_$latest.csv" | grep .csv | xargs rm

# Renames final CSVs
nummemorynodes=$(numactl --hardware | head -n 1 | cut -d ' ' -f 2)

if [ $nummemorynodes -eq 1 ]
then
	rename "s/_$latest//g" *.csv # This works in local
else
	rename "_$latest" "" *.csv # This works in server
fi

