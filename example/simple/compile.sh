# -D LIBMICROKITCO_UNSAFE

CPU='cortex-a55'
MICROKIT_BOARD='odroidc4'

CPU='cortex-a53'
MICROKIT_BOARD='qemu_arm_virt'


aarch64-linux-gnu-gcc -c -mcpu="$CPU" -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -w ./../../libco/libco.c -o build/libco.o &&

aarch64-linux-gnu-gcc -c -mcpu="$CPU" -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -Werror -I./../../sdk/board/"$MICROKIT_BOARD"/debug/include -Iinclude -DBOARD_"$MICROKIT_BOARD" ./../../libmicrokitco.c -o build/libmicrokitco.o &&

aarch64-linux-gnu-gcc -c -mcpu="$CPU" -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -Werror -I./../../sdk/board/"$MICROKIT_BOARD"/debug/include -I./../../ -Iinclude -DBOARD_"$MICROKIT_BOARD" client.c -o build/client.o &&

aarch64-linux-gnu-ld -L../../sdk/board/"$MICROKIT_BOARD"/debug/lib build/libmicrokitco.o build/client.o build/libco.o -lmicrokit -Tmicrokit.ld -o build/client.elf &&

../../sdk/bin/microkit simple.system --search-path build --board "$MICROKIT_BOARD" --config debug -o build/loader.img -r build/report.txt &&

# mq.sh run -s odroidc4_1 -f build/loader.img -c "CLIENT: done, exiting"

qemu-system-aarch64 -machine virt,virtualization=on \
    -cpu "$CPU" \
    -serial mon:stdio \
    -device loader,file=build/loader.img,addr=0x70000000,cpu-num=0 \
    -m size=2G \
    -nographic \
    -netdev user,id=mynet0 \
    -device virtio-net-device,netdev=mynet0,mac=52:55:00:d1:55:01

echo done
