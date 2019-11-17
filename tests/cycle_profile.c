#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char ** argv)
{
	//register volatile uint64_t cycle_start asm ("t0") = 0;
	//register volatile uint64_t cycle_end asm ("t1") = 0;
	volatile uint64_t cycle_start = 0;
	volatile uint64_t cycle_end = 0;
	int a = 3;
	int b = 4;
	volatile int c = 0;
	//asm volatile("csrrw zero, cycle, zero");
	asm volatile("rdcycle %0" : "=r"(cycle_start));
	c = a + b;
	asm volatile("rdcycle %0" : "=r"(cycle_end));
	asm volatile("fence");
	printf("C: %d\n", c);
	printf("Cycles start: %u\n", cycle_start);
	printf("Cycles end: %u\n", cycle_end);
	printf("Cycles elapsed: %u\n", cycle_end - cycle_start);

	return 0;
}
