# Change these for your setup!
MICROKIT_SDK='/home/billn/microkit-sdk-1.2.6-patched'
MICROKIT_X86_64_SDK='/home/billn/microkit-sdk-1.2.6-x86-64'
A64N_TOOLCHAIN_PATH_PREFIX='/opt/toolchain/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf/bin/'
A64L_TOOLCHAIN_PATH_PREFIX=''
R64_TOOLCHAIN_PATH_PREFIX='/opt/toolchain/riscv/bin/'
X64_TOOLCHAIN_PATH_PREFIX=''

echo "Choose your system to run:"
echo "1. QEMU aarch64 - Build with LLVM"
echo "2. QEMU x86_64  - Build with LLVM"
echo "3. QEMU riscv64 - Build with LLVM"
echo "4. QEMU aarch64 - Build with aarch64-none-elf-gcc"
echo "5. QEMU aarch64 - Build with aarch64-linux-gnu-gcc"
echo "6. QEMU x86_64  - Build with x86_64-linux-gnu-gcc"
echo "7. QEMU riscv64 - Build with riscv64-unknown-elf-gcc"

echo -n "Enter your option: "
read option
if [ $option = 1 ] 
then
    make run_qemu_aarch64 TARGET=aarch64-none-elf MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 2 ]
then
    make run_qemu_x86_64 TARGET=x86_64-none-elf MICROKIT_SDK="$MICROKIT_X86_64_SDK"

elif [ $option = 3 ]
then
    make run_qemu_riscv64 TARGET=riscv64-none-elf MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 4 ]
then
    make run_qemu_aarch64 TARGET=aarch64-none-elf TOOLCHAIN="${A64N_TOOLCHAIN_PATH_PREFIX}aarch64-none-elf" MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 5 ]
then
    make run_qemu_aarch64 TARGET=aarch64-linux-gnu TOOLCHAIN="${A64L_TOOLCHAIN_PATH_PREFIX}aarch64-linux-gnu" MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 6 ]
then
    make run_qemu_x86_64 TARGET=x86_64-linux-gnu TOOLCHAIN="${X64_TOOLCHAIN_PATH_PREFIX}x86_64-linux-gnu" MICROKIT_SDK="$MICROKIT_X86_64_SDK"

elif [ $option = 7 ]
then
    make run_qemu_riscv64 TARGET=riscv64-unknown-elf TOOLCHAIN="${R64_TOOLCHAIN_PATH_PREFIX}riscv64-unknown-elf" MICROKIT_SDK="$MICROKIT_SDK"

fi
