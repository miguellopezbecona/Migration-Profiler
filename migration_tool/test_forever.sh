#!/bin/bash

profname=my_profiler_tm
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

# Executes app to profile in background along with the profiler
./$profname $profparams &
echo "Profiler launched. End it cleanly with \"kill -2 $!\" or \"pkill -2 $profname\""

