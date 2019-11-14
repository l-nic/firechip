#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

register volatile uint64_t global_ptr1 asm ("t5");
register volatile uint64_t global_ptr2 asm ("t6");

void lnic_set_enable(uint64_t enable) {
	asm("nicwrite zero, a0, zero");
}

void lnic_set_own_id(uint64_t own_id) {
	asm("csrrw zero, lownid, a0");
}

uint64_t lnic_get_own_id() {
	register uint64_t to_return asm("a0");
	asm("csrrw a0, lownid, zero");
	return to_return;
}

uint64_t lnic_read_word() {
	register uint64_t to_return asm("a0");
	asm("cssrw a0, lread, zero");
	return to_return;
}

uint64_t lnic_messages_ready() {
	register uint64_t to_return asm("a0");
	asm("cssrw a0, lmsgsrdy, zero");
	return to_return;
}

uint64_t lnic_is_last_word() {
	register uint64_t to_return asm("a0");
	asm("cssrw a0, lrdend, zero");
	return to_return;
}

uint64_t lnic_src_ip_lower() {
	register uint64_t to_return asm("a0");
	asm("cssrw a0, lrdsrciplo, zero");
	return to_return;
}

uint64_t lnic_src_ip_upper() {
	register uint64_t to_return asm("a0");
	asm("cssrw a0, lrdsrciphi, zero");
	return to_return;
}

uint64_t lnic_src_port() {
	register uint64_t to_return asm("a0");
	asm("cssrw a0, lrdsrcprt, zero");
	return to_return;
}

void lnic_write_word() {
	asm("cssrw zero, lwrite, a0");
}

void lnic_write_message_end() {
	asm("cssrw zero, lwrend, a0");
}

void lnic_set_dst_ip_lower(uint64_t ip_lower_bits) {
	asm("cssrw zero, lwrdstiplo, a0");
}

void lnic_set_dst_ip_upper(uint64_t ip_upper_bits) {
	asm("cssrw zero, lwrdstiphi, a0");
}

void lnic_set_dst_port(uint64_t dst_port) {
	asm("cssrw zero, lwrdstprt, a0");
}

int main(int argc, char ** argv) {
	if (argc != 1) {
		return ARGUMENT_ERROR;
	}
	lnic_set_enable(true);
	lnic_set_own_id(1);
	while (lnic_messages_ready() == 0);
	uint64_t input_data = lnic_read_word();
	lnic_set_dst_ip_lower(lnic_src_ip_lower());
	lnic_set_dst_port(lnic_src_port());
	lnic_write_word(input_data + 1);
	lnic_write_message_end();
	while (true) {
		while (lnic_messages_ready() == 0);
		lnic_write_word(lnic_read_word() + 1);
		lnic_write_message_end();
	}
	lnic_set_enable(false);
	return 0;
}
