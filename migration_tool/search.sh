#!/bin/bash

grep -n $1 *.c *.h perfmon/*.* migration/*.* strategies/*.* strategies/genetic/*.* rapl/*.* --color=always
