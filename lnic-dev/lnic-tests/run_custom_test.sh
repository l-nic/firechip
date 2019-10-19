# Build the test (This will only work for 64-bit UI, with no virtual memory,
# for now).
if [ ! -d "build" ]; then
	mkdir build
fi
riscv64-unknown-elf-gcc -march=rv64g -mabi=lp64 -I./env/p -I./macros/scalar \
	-E rv64ui/$@.S | \
	/home/vagrant/firechip/lnic-dev/binutils-gdb/build/gas/as-new \
	-march=rv64g -mabi=lp64 -I./env/p -I./macros/scalar    \
	- -o build/$@.o

# Link the test
riscv64-unknown-elf-ld -T./env/p/link.ld -o build/$@.riscv build/$@.o

# Run the test in spike
/home/vagrant/firechip/lnic-dev/riscv-isa-sim/build/spike build/$@.riscv


