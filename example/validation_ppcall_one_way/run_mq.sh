
if [ "$1" = "odroidc4" ];
then
    rm -rfd build && \
    make build_odroidc4 TOOLCHAIN='/opt/toolchain/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf/bin/aarch64-none-elf' MICROKIT_SDK='/home/billn/microkit-sdk-1.2.6-patched' && \
    mq.sh run -s odroidc4_1 -f build/loader.img -c "BENCHFINISHED" && exit 0

fi

if [ "$1" = "hifive" ];
then
    rm -rfd build && \
    make build_hifive TOOLCHAIN='/home/billn/riscv64-unknown-elf-toolchain-10.2.0-2020.12.8-x86_64-linux-ubuntu14/bin/riscv64-unknown-elf'  MICROKIT_SDK='/home/billn/microkit-sdk-1.2.6-patched' && \
    make -C /home/billn/opensbi -j$(nproc) PLATFORM=generic FW_PAYLOAD_PATH=$(pwd)/build/loader.img PLATFORM_RISCV_XLEN=64 PLATFORM_RISCV_ISA=rv64imac PLATFORM_RISCV_ABI=lp64 O=$(pwd)/build CROSS_COMPILE='/home/billn/riscv64-unknown-elf-toolchain-10.2.0-2020.12.8-x86_64-linux-ubuntu14/bin/riscv64-unknown-elf-' && \
    mq.sh run -s hifive -f build/platform/generic/firmware/fw_payload.bin -c "BENCHFINISHED" && exit 0
fi

echo "unknown board"
exit 1
