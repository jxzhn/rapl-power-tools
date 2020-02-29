#!/bin/bash
counter=0
for powerLimit in $(seq 66 2 126); do
    let counter+=1
    echo "Set power limit to ${powerLimit}."
    rapl-set -p 0 -c 1 -l ${powerLimit}000000
    rapl-set -p 1 -c 1 -l ${powerLimit}000000
    ./powercap-demo -i 50 -f ${counter} & # 运行RAPL检测程序并分离
    raplPID=`pgrep demo` # GET一下RAPL监测程序的PID，便于及时kill掉
    mpirun -n 2 ./xhpl_intel64_static > log_${counter}.txt
    kill raplPID
    sleep 0.5
done
# 恢复RAPL默认限制，务必提前用rapl-info读取默认值
# short-term
rapl-set -p 0 -c 1 -l 126000000
rapl-set -p 1 -c 1 -l 126000000
# long-term
rapl-set -p 0 -c 0 -l 105000000
rapl-set -p 1 -c 0 -l 105000000
