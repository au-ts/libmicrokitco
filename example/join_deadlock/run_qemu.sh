# You must define these
# MICROKIT_SDK=''
# MICROKIT_X86_64_SDK=''

# These can be left empty if the toolchains are in your $PATH
# A64N_TOOLCHAIN_PATH_PREFIX=''
# A64L_TOOLCHAIN_PATH_PREFIX=''
# R64_TOOLCHAIN_PATH_PREFIX=''
# X64_TOOLCHAIN_PATH_PREFIX=''

echo "Choose your system to run:"
echo "1. QEMU aarch64 - Build with LLVM"
echo "2. QEMU riscv64 - Build with LLVM"
echo "3. QEMU aarch64 - Build with aarch64-none-elf-gcc"
echo "4. QEMU aarch64 - Build with aarch64-linux-gnu-gcc"
echo "5. QEMU riscv64 - Build with riscv64-unknown-elf-gcc"

echo -n "Enter your option: "
read option
if [ $option = 1 ] 
then
    make run_qemu_aarch64 TARGET=aarch64-none-elf MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 2 ]
then
    make run_qemu_riscv64 TARGET=riscv64-none-elf MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 3 ]
then
    make run_qemu_aarch64 TARGET=aarch64-none-elf TOOLCHAIN="${A64N_TOOLCHAIN_PATH_PREFIX}aarch64-none-elf" MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 4 ]
then
    make run_qemu_aarch64 TARGET=aarch64-linux-gnu TOOLCHAIN="${A64L_TOOLCHAIN_PATH_PREFIX}aarch64-linux-gnu" MICROKIT_SDK="$MICROKIT_SDK"

elif [ $option = 5 ]
then
    make run_qemu_riscv64 TARGET=riscv64-unknown-elf TOOLCHAIN="${R64_TOOLCHAIN_PATH_PREFIX}riscv64-unknown-elf" MICROKIT_SDK="$MICROKIT_SDK"

fi
