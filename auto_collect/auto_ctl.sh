#!/bin/bash

counter=0
for powerLimit in $(seq 66 2 126); do # Notice that this should be changed for different machines
    let counter+=1
    procBegin=false
    echo "Setting power limit to ${powerLimit}."
    rapl-set -p 0 -c 1 -l ${powerLimit}000000
    rapl-set -p 1 -c 1 -l ${powerLimit}000000
    ./read-power -i 50 -f ${counter} & # Run the tool to read power and detach it
    raplPID=`pgrep demo` # since we detach it, we need its PID to kill it
    touch ready # to tell the other script that the power limit is set, and ready to run HPL
    while [ 1 ]; do
        if [ -f running ]; then # HPL is running now ("running" file is created by the other script)
            if [ "$procBegin" == "false" ]; then # del "ready" file to prevent next HPL run before really be ready 
                procBegin=true
                rm ready
            fi
        elif [ "$procBegin" == "true" ]; then # HPL has ended now
            kill ${raplPID}
            usleep 5000 # It seems that "kill" is asynchronous, just wait it for 5ms
            break
        fi
        sleep 1
    done
done
# Recover the default RAPL parameters(This is not the same for different machines)
rapl-set -p 0 -c 1 -l 126000000
rapl-set -p 1 -c 1 -l 126000000
rapl-set -p 0 -c 0 -l 105000000
rapl-set -p 1 -c 0 -l 105000000