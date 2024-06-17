#!/bin/bash

# Change these if necessary for your system.
# For example, this is my setup:
export A64_TOOLCHAIN='/opt/toolchain/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf/bin/aarch64-none-elf'
export R64_TOOLCHAIN='/home/billn/riscv64-unknown-elf-toolchain-10.2.0-2020.12.8-x86_64-linux-ubuntu14/bin/riscv64-unknown-elf'

# Don't change these
export SDK=$(realpath microkit-sdk-1.2.6-linux_x86_64-libmicrokitco-bench-riscv_patched)
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
