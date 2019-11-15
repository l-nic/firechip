riscv64-unknown-elf-gcc -S -fverbose-asm riscv_trivial_loopback.c

# Use the custom assembler to assemble to object code
/home/vagrant/firechip/lnic-dev/binutils-gdb/build/gas/as-new riscv_trivial_loopback.s -o riscv_trivial_loopback.o

# Link the object code into the final binary
riscv64-unknown-elf-ld -L/opt/riscv/lib/gcc/riscv64-unknown-elf/7.2.0 /opt/riscv/riscv64-unknown-elf/lib/crt0.o /opt/riscv/lib/gcc/riscv64-unknown-elf/7.2.0/crtbegin.o riscv_trivial_loopback.o -lgcc -lc -lgloss -lc /opt/riscv/lib/gcc/riscv64-unknown-elf/7.2.0/crtend.o -o riscv_trivial_loopback.riscv

/home/vagrant/firechip/lnic-dev/riscv-isa-sim/build/spike --nic-config-data=127.0.0.1 /home/vagrant/firechip/lnic-dev/riscv-pk/build/pk riscv_trivial_loopback.riscv