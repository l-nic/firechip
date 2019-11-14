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
#define CONFIG_TYPE 4

#define ARGUMENT_ERROR -3
#define CONFIG_ERROR -4

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

// TODO: If we end up with enough random configuration registers,
// we might want to start serializing some of this data.
static inline uint64_t lnic_reserve_port(uint64_t port) {
	// Reserve port if possible, and return it if success
	// Return the next unused port if failure
	// The reservation binds this thread's id to the port number
	// We need something like this so that other applications can send
	// messages to this one without knowing the thread id in advance
	// They'll only need to know the port number, which can be pre-defined
	// for a particular service.
}

static inline uint64_t lnic_free_port(uint64_t port) {

}

// IPv4 32-bits in lower 32 bits here.
static inline void lnic_set_dest_ip_lower(uint64_t ip) {

}

// Only used for ipv6.
static inline void lnic_set_dest_ip_upper(uint64_t ip) {

}

static inline void lnic_set_dest_lnic_port(uint64_t port) {

}

// TODO: We'll eventually need a way to configure the ip's too

int run_loopback() {
	// Configure our own id and the loopback.
	lnic_set_own_id(1);
	while (lnic_messages_ready() == 0);
	uint64_t message_type = lnic_read_word();
	if (message_type != CONFIG_TYPE) {
		return CONFIG_ERROR;
	}
	while (!lnic_read_message_end()) {
		lnic_read_word();
	}
	lnic_set_dest_ip_lower(lnic_read_message_ip_lower());
	lnic_set_dest_port(lnic_read_message_src_port());

	// Receive messages and send back responses. The lnic assumes that each
	// message type can fit into a single packet, but makes no assumptions beyond that.

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
