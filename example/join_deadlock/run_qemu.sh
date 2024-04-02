# -D LIBMICROKITCO_UNSAFE

CPU='cortex-a53'

rm -rfd build && make && qemu-system-aarch64 -machine virt,virtualization=on \
    -cpu "$CPU" \
    -serial mon:stdio \
    -device loader,file=build/loader.img,addr=0x70000000,cpu-num=0 \
    -m size=2G \
    -nographic \
    -netdev user,id=mynet0 \
    -device virtio-net-device,netdev=mynet0,mac=52:55:00:d1:55:01

echo done
