#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ARR_SIZE 2048 // Size is in words, so this is in Kbytes

volatile uint64_t cycle_start = 0;
volatile uint64_t cycle_end = 0;
volatile uint64_t inst_start = 0;
volatile uint64_t inst_end = 0;
volatile uint64_t arr[ARR_SIZE];
uint64_t i = 0;
uint64_t global_index = 0;

void run_trial(uint64_t arr_size) {
	asm volatile("rdinstret %0" : "=r"(inst_start));
	asm volatile("rdcycle %0" : "=r"(cycle_start));
	for (i = global_index; i < arr_size + global_index; i++) {
		arr[i]++;
	}
	asm volatile("rdcycle %0" : "=r"(cycle_end));
	asm volatile("rdinstret %0" : "=r"(inst_end));
	printf("%u,%u,%u\n", arr_size, inst_end - inst_start, cycle_end - cycle_start);
	global_index = i;
	//printf("Instructions start: %u, end: %u, elapsed: %u\n", inst_start, inst_end, inst_end - inst_start);
	//printf("Cycles start: %u, end: %u, elapsed: %u\n", cycle_start, cycle_end, cycle_end - cycle_start);
}

int main() {
	run_trial(100);
	run_trial(150);
	run_trial(200);
	run_trial(300);
	global_index = 0;
	run_trial(300);
	return 0;
}
