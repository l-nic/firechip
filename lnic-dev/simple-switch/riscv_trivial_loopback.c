#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

register volatile uint64_t global_ptr1 asm ("t5");
register volatile uint64_t global_ptr2 asm ("t6");

int main(int argc, char ** argv) {
	if (argc != 1) {
		return ARGUMENT_ERROR;
	}
	lnic_set_enable(true);
	lnic_set_own_id(1);
	while (lnic_messages_ready() == 0);
	uint64_t input_data = lnic_read_word();
	lnic_set_dest_ip_lower(lnic_read_message_ip_lower());
	lnic_set_dest_port(lnic_read_message_src_port());
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
