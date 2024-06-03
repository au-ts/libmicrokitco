#!/bin/sh

for benchmark in example/validation_*
do
    (
        # Run Odroid C4
        cd benchmark && \
        ./run_mq.sh odroidc4 >report.txt && \
        echo "Odroid C4 - $benchmark:" && \
        grep -E "(Mean|Stdev)" <report.txt && exit 0
    ) 

    if [ $? == 0 ];
    then

        (
            # Run HiFive
            cd benchmark && \
            ./run_mq.sh hifive >report.txt && \
            echo "HiFive Unleashed - $benchmark:" && \
            grep -E "(Mean|Stdev)" <report.txt && exit 0
        ) 

    else

        break

    fi
done