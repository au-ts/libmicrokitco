#!/bin/bash

export A64_TOOLCHAIN=''
export R64_TOOLCHAIN=''
export SDK=''
export OPENSBI=$(realpath opensbi)

run_odroidc4 () {
    (
        # Run Odroid C4
        cd $benchmark && \
        ./run_mq.sh odroidc4 &>report.txt && \
        echo "Odroid C4 - $benchmark:" && \
        grep -E "(Mean|Stdev)" <report.txt && exit 0
    )
}

run_hifive () {
    (
        # Run HiFive
        cd $benchmark && \
        ./run_mq.sh hifive &>report.txt && \
        echo "HiFive Unleashed - $benchmark:" && \
        grep -E "(Mean|Stdev)" <report.txt && exit 0
    ) 
}

for benchmark in validation_*
do
    if [ "$1" = "odroidc4" ];
    then
        run_odroidc4
    elif [ "$1" = "hifive" ];
    then
        run_hifive
    elif [ "$1" = "both" ];
    then
        run_odroidc4
        run_hifive
    else
        echo "unknown option"
        break
    fi
done
