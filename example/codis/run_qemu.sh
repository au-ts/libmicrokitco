# Change these for your setup!
MICROKIT_SDK='/Users/dreamliner787-9/TS/sdk'
MICROKIT_X86_64_SDK='/Users/dreamliner787-9/TS/sdk_x86_64'
TOOLCHAIN_PATH_PREFIX=''

echo "Choose your system to run:"
echo "1. QEMU aarch64 - Build with LLVM"
echo "2. QEMU riscv64 - Build with LLVM"
echo "3. QEMU aarch64 - Build with aarch64-none-elf triple"
echo "4. QEMU aarch64 - Build with aarch64-unknown-linux-gnu triple"
echo "5. QEMU riscv64 - Build with riscv64-unknown-elf triple"

echo "Enter your option: \c"
read option
if [ $option = 1 ] 
then
    make run_qemu_aarch64 TARGET=aarch64-none-elf MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 2 ]
then
    make run_qemu_riscv64 TARGET=riscv64-none-elf MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 3 ]
then
    make run_qemu_aarch64 TARGET=aarch64-none-elf TOOLCHAIN="${TOOLCHAIN_PATH_PREFIX}aarch64-none-elf" MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 4 ]
then
    make run_qemu_aarch64 TARGET=aarch64-unknown-linux-gnu TOOLCHAIN="${TOOLCHAIN_PATH_PREFIX}aarch64-unknown-linux-gnu" MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 5 ]
then
    make run_qemu_riscv64 TARGET=riscv64-unknown-elf TOOLCHAIN="${TOOLCHAIN_PATH_PREFIX}riscv64-unknown-elf" MICROKIT_SDK="$MICROKIT_SDK"

fi
