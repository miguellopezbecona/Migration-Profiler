#!/bin/bash

execname=my_profiler_tm
profparams="-s1000 -l250 -p650 -P100000000"
#profparams="-s500 -l50 -p300 -P50000000"
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

# Set to what you want to profile. A bit deprecated since now the profiler analyzes everything
#toprofile="numactl --physcpubind 1,2,3 ./ABC -m0 -r5000 -s5000000 -t8"
toprofile=~/NPB3.3.1/NPB3.3-OMP/bin/lu.B.x
#toprofile=~/NPB3.3.1/NPB3.3-OMP/bin/bt.C.x

# Executes app to profile in background along with the profiler
nohup $toprofile &>/dev/null &
$numacommand ./$execname $profparams

# Does not continue if there was an error
if [ $? -ne 0 ]; then
	exit $?
fi

exit 1

# Does file preprocessing if any CSV file exists
ls .csv &> /dev/null
if [ $? -ne 0 ]; then
	exit 0
fi


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

