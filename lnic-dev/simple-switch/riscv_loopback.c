#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// This defines the message format but will never be explicitly
// used in the code.
// struct FixedMessage {
// 	uint64_t message_type;
//  uint64_t response_type;
// 	uint64_t[2] message_data;
// };

// struct VariableMessage {
// 	uint64_t message_type;
// 	uint64_t response_type;
// 	uint64_t* variable_message_data;
// };

// struct TerminateMessage {
// 	uint64_t message_type;
// };

#define INVALID_TYPE 0
#define FIXED_TYPE 1
#define VARIABLE_TYPE 2
#define TERMINATE_TYPE 3

#define ARGUMENT_ERROR -3

register volatile uint64_t global_ptr1 asm ("t5");
register volatile uint64_t global_ptr2 asm ("t6");

bool lnic_ready() {
	register uint64_t to_return asm("a0");
	asm("nicread a0, zero, zero");
	return to_return;
}

void lnic_set_enable(uint64_t enable) {
	asm("nicwrite zero, a0, zero");
}

static inline void lnic_write_word(uint64_t word) {
	asm("mv x30, a0");
}

static inline uint64_t lnic_read_word() {
	register uint64_t to_return asm("a0");
	asm("mv a0, x31");
	return to_return;
}

static inline void lnic_write_message_end() {

}

static inline bool lnic_read_message_end() {

}

static inline uint64_t lnic_messages_ready() {
	
}

int run_loopback() {
	while (true) {
		while (lnic_messages_ready() == 0) { // We're not ready to use word-count yet, since that requires blocking reads in case the message hasn't fully arrived
			// Wait for at least one message to be in the queue
		}
		uint64_t message_type = lnic_read_word();
		if (message_type == FIXED_TYPE) {
			lnic_write_word(lnic_read_word()); // Send the response type
			uint64_t message_data_word1 = lnic_read_word();
			uint64_t message_data_word2 = lnic_read_word();
			lnic_write_word(FIXED_TYPE); // These are just replying to a test fixture. It doesn't need to respond again.
			lnic_write_word(message_data_word1*message_data_word2); // This will fit in both the fixed and variable messages.
			lnic_write_word(message_data_word1+message_data_word2);
			lnic_write_message_end();
		} else if (message_type == VARIABLE_TYPE) {
			lnic_write_word(lnic_read_word());
			lnic_write_word(FIXED_TYPE);
			while (!lnic_read_message_end()) {
				uint64_t message_data_word1 = lnic_read_word();
				if (lnic_read_message_end()) {
					break;
				}
				uint64_t message_data_word2 = lnic_read_word();
				lnic_write_word(message_data_word1*message_data_word2); // This will fit in both the fixed and variable messages.
				lnic_write_word(message_data_word1+message_data_word2);
			}
			lnic_write_message_end();
		} else if (message_type == TERMINATE_TYPE) {
			while (!lnic_read_message_end()) {
				lnic_read_word();
			}
			break;
		}
		// Always read to the end of the message to purge invalid data/messages
		while (!lnic_read_message_end()) {
			lnic_read_word();
		}
	}
	return 0;
}

int main(int argc, char ** argv) {
	printf("Starting riscv loopback test\n");
	if (argc != 1) {
		return ARGUMENT_ERROR;
	}
	lnic_set_enable(true); // TODO: Verify that these are even
	// needed if the trap state restore is removed
	int retval = run_loopback();
	lnic_set_enable(false);
	return retval;
}
