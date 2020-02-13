#!/bin/bash

counter=0
while [ 1 ]; do
    if [ -f ready ]; then # The power limit is set, and ready for running now("ready" file is created by the other script)
        touch running # To tell the other script HPL is running
        let counter+=1
        echo "Run time counter=${counter}"
        mpirun -n 2 ./xhpl_intel64_static > log_${counter}.txt
        rm running # Now it is finined
    fi
    sleep 5
done
