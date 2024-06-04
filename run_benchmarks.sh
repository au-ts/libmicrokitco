#!/bin/sh

export A64_TOOLCHAIN='/opt/toolchain/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf/bin/aarch64-none-elf'
export R64_TOOLCHAIN='/opt/toolchain/riscv/bin/riscv64-unknown-elf'
export SDK='/home/billn/microkit-sdk-1.2.6-patched'
export OPENSBI='/home/billn/opensbi'

for benchmark in example/validation_*
do
    (
        # Run Odroid C4
        cd $benchmark && \
        ./run_mq.sh odroidc4 >report.txt && \
        echo "Odroid C4 - $benchmark:" && \
        grep -E "(Mean|Stdev)" <report.txt && exit 0
    ) 

    if [ $? = 0 ];
    then

        (
            # Run HiFive
            cd $benchmark && \
            ./run_mq.sh hifive >report.txt && \
            echo "HiFive Unleashed - $benchmark:" && \
            grep -E "(Mean|Stdev)" <report.txt && exit 0
        ) 

    else

        break

    fi
done
