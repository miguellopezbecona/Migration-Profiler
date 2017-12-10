#!/bin/bash

profname=rapl
profparams="-s1"

# Goes to source code folder if you execute script from another directory
dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $dir

# Compiles test program
gcc -O0 -fopenmp -o my_test my_test.c system_struct.c -lnuma

# Compiles profiler
make

# Exits if it there was a problem
if [ $? -ne 0 ]; then
	exit 1
fi

# Set to what you want to profile
toprofile="./my_test -l10000000 -d4 -r0 -o500 -t1 -m0 -M0"
#toprofile=~/NPB3.3.1/NPB3.3-OMP/bin/lu.B.x
#toprofile=~/NPB3.3.1/NPB3.3-OMP/bin/bt.C.x

# Executes app to profile in background along with the profiler
./$profname $profparams &
$toprofile > /dev/null

pkill -2 $profname # Ends profiler cleanly when the other app finishes

