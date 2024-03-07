# -D LIBMICROKITCO_UNSAFE
aarch64-linux-gnu-gcc -c -mcpu=cortex-a53 -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -w ./../../libco/libco.c -o build/libco.o &&

aarch64-linux-gnu-gcc -c -mcpu=cortex-a53 -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -Werror -I./../../sdk/board/qemu_arm_virt/debug/include -Iinclude -DBOARD_qemu_arm_virt ./../../libmicrokitco.c -o build/libmicrokitco.o &&

aarch64-linux-gnu-gcc -c -mcpu=cortex-a53 -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -Werror -I./../../sdk/board/qemu_arm_virt/debug/include -I./../../ -Iinclude -DBOARD_qemu_arm_virt client.c -o build/client.o &&

aarch64-linux-gnu-ld -L../../sdk/board/qemu_arm_virt/debug/lib build/libmicrokitco.o build/client.o build/libco.o -lmicrokit -Tmicrokit.ld -o build/client.elf &&

../../sdk/bin/microkit simple.system --search-path build --board qemu_arm_virt --config debug -o build/loader.img -r build/report.txt &&

qemu-system-aarch64 -machine virt,virtualization=on \
    -cpu cortex-a53 \
    -serial mon:stdio \
    -device loader,file=build/loader.img,addr=0x70000000,cpu-num=0 \
    -m size=2G \
    -nographic \
    -netdev user,id=mynet0 \
    -device virtio-net-device,netdev=mynet0,mac=52:55:00:d1:55:01

echo done
