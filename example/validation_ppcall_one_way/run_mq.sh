
if [ "$1" = "odroidc4" ];
then
    rm -rfd build && \
    make build_odroidc4 && \
    mq.sh run -s odroidc4_1 -f build/loader.img -c "BENCHFINISHED" && exit 0

fi

if [ "$1" = "hifive" ];
then
    rm -rfd build && \
    make build_hifive && \
    make -C /home/billn/opensbi -j$(nproc) PLATFORM=generic FW_PAYLOAD_PATH=$(pwd)/build/loader.img PLATFORM_RISCV_XLEN=64 PLATFORM_RISCV_ISA=rv64imac PLATFORM_RISCV_ABI=lp64 O=$(pwd)/build CROSS_COMPILE=riscv-none-elf- && \
    mq.sh run -s hifive -f build/platform/generic/firmware/fw_payload.bin -c "BENCHFINISHED" && exit 0
fi

echo "unknown board"
exit 1
