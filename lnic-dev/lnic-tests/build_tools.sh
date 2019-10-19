# Generate the opcodes
cd /home/vagrant/firechip/lnic-dev/riscv-opcodes
make install

# Build the custom assembler
cd /home/vagrant/firechip/lnic-dev/binutils-gdb
if [ ! -d "build" ]; then
	mkdir build
fi
cd build
if [ -z "$(ls -A .)" ]; then
	../configure --target=riscv64-unknown-elf --prefix=$RISCV
fi
make -j4

# Build the custom spike simulator
cd /home/vagrant/firechip/lnic-dev/riscv-isa-sim
if [ ! -d "build" ]; then
	mkdir build
fi
cd build
if [ -z "$(ls -A .)" ]; then
	../configure --prefix=$RISCV
fi
make -j4


