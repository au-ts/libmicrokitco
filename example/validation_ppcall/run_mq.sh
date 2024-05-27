
if [ "$1" = "odroidc4" ];
then

    sed -E 's/phys_addr=\"0x[0-9a-f]{4}_[0-9a-f]{4}/phys_addr=\"0xff80_3000/' validation_ppcall.system >temp.system && \
    mv temp.system validation_ppcall.system && \
    rm -rfd build && \
    make TARGET=aarch64-none-elf MICROKIT_BOARD=odroidc4 CPU=cortex-a55 ECFLAGS='-mcpu=cortex-a55' SERIAL=serial_drv SERIAL_CONFIG=-DCONFIG_PLAT_ODROIDC4 && \
    mq.sh run -s odroidc4_1 -f build/loader.img -c "BENCHFINISHED" && exit 0

fi

if [ "$1" = "hifive" ];
then

    sed -E 's/phys_addr=\"0x[0-9a-f]{4}_[0-9a-f]{4}/phys_addr=\"0x1001_0000/' validation_ppcall.system >temp.system && \
    mv temp.system validation_ppcall.system && \
    rm -rfd build && \
    make TARGET=riscv64-none-elf MICROKIT_BOARD=star64 CPU=medany ECFLAGS='-mcmodel=medany -march=rv64imac -mabi=lp64' SERIAL=serial_drv SERIAL_CONFIG=-DCONFIG_PLAT_STAR64 && \
    mq.sh run -s star64 -f build/loader.img -c "BENCHFINISHED" && exit 0

fi

echo "unknown board"
exit 1
