if [ "$1" = "odroidc4" ];
then
    rm -rfd build && \
    make build_odroidc4 TOOLCHAIN="$A64_TOOLCHAIN" TARGET='aarch64-none-elf' MICROKIT_SDK="$SDK"

    if [ "$?" != 0 ];
    then
        echo "Build FAILED!" && exit 1
    else
        mq.sh run -s odroidc4_1 -f build/loader.img -c "BENCHFINISHED"
        exit 0
    fi
fi

if [ "$1" = "hifive" ];
then
    rm -rfd build && \
    make build_hifive TOOLCHAIN="$R64_TOOLCHAIN" TARGET='riscv64-unknown-elf' MICROKIT_SDK="$SDK"

    if [ "$?" != 0 ];
    then
        echo "Build FAILED!" && exit 1
    else
        mq.sh run -s hifive -f build/platform/generic/firmware/fw_payload.bin -c "BENCHFINISHED"
        exit 0
    fi
fi

echo "unknown board"
exit 1
