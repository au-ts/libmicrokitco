A64_TOOLCHAIN='/opt/toolchain/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf/bin/aarch64-none-elf'
R64_TOOLCHAIN='/home/billn/riscv64-unknown-elf-toolchain-10.2.0-2020.12.8-x86_64-linux-ubuntu14/bin/riscv64-unknown-elf'
OPENSBI='/home/billn/opensbi'

SDK='/home/billn/microkit-sdk-1.2.6-patched'

if [ "$1" = "odroidc4" ];
then
    rm -rfd build && \
    make build_odroidc4 TOOLCHAIN="$A64_TOOLCHAIN" MICROKIT_SDK="$SDK"

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
    make build_hifive TOOLCHAIN="$R64_TOOLCHAIN" MICROKIT_SDK="$SDK"

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
