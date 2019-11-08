riscv64-unknown-elf-gcc -S -fverbose-asm riscv_othello_node.c

# Use the custom assembler to assemble to object code
/home/vagrant/firechip/lnic-dev/binutils-gdb/build/gas/as-new riscv_othello_node.s -o riscv_othello_node.o

# Link the object code into the final binary
riscv64-unknown-elf-ld -L/opt/riscv/lib/gcc/riscv64-unknown-elf/7.2.0 /opt/riscv/riscv64-unknown-elf/lib/crt0.o /opt/riscv/lib/gcc/riscv64-unknown-elf/7.2.0/crtbegin.o riscv_othello_node.o -lgcc -lc -lgloss -lc /opt/riscv/lib/gcc/riscv64-unknown-elf/7.2.0/crtend.o -o riscv_othello_node.riscv
