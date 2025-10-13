# You must define this, either here or as part of your environment.
# MICROKIT_SDK=''

echo "Choose your system to run:"
echo "1. QEMU aarch64 - Build with LLVM"
echo "2. QEMU x86_64  - Build with LLVM"
echo "3. QEMU riscv64 - Build with LLVM"

echo -n "Enter your option: "
read option
if [ $option = 1 ] 
then
    make run_qemu_aarch64 TARGET=aarch64-none-elf MICROKIT_SDK="$MICROKIT_SDK" MICROKIT_BOARD=qemu_virt_aarch64

elif [ $option = 2 ]
then
    make run_qemu_x86_64 TARGET=x86_64-none-elf MICROKIT_SDK="$MICROKIT_SDK" MICROKIT_BOARD=x86_64_generic

elif [ $option = 3 ]
then
    make run_qemu_riscv64 TARGET=riscv64-none-elf MICROKIT_SDK="$MICROKIT_SDK" MICROKIT_BOARD=qemu_virt_riscv64
fi
