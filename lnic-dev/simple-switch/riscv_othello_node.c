#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// These define the message formats but will never be explicitly
// used in the code.
// struct Message {
// 	uint64_t message_type_id;
// 	uint64_t destination_id;
// };

// struct MapMessage {
// 	uint64_t message_type_id;
// 	uint64_t destination_id;
// 	uint64_t max_depth;
// 	uint64_t cur_depth;
// 	uint64_t src_host_id;
// };

// struct ReduceMessage {
// 	uint64_t message_type_id;
// 	uint64_t destination_id;
// };

#define BRANCHING_FACTOR 4
#define MAX_DEPTH 3
#define INVALID_ID 0
#define MAP_ID 1
#define REDUCE_ID 2
#define ROOT_ID 1

#define ID_MISMATCH -1
#define UNRECOGNIZED_MESSAGE -2
#define ARGUMENT_ERROR -3

register volatile uint64_t global_ptr1 asm ("t5");
register volatile uint64_t global_ptr2 asm ("t6");

// These fields are logically part of the message state struct
// but will be referred to without the added indirection layer.
// struct MessageState {
//	uint64_t response_cnt; // Reduce responses received so far
//	uint64_t map_cnt; // Total number of map requests sent. (<= BRANCHING_FACTOR)
//	uint64_t src_host_id; // ID of the server that sent us a map message.
// };

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

static inline uint64_t get_child_id(uint64_t own_id, uint64_t i) {
	return (BRANCHING_FACTOR * own_id) - (BRANCHING_FACTOR - 2) + i;
}

int main(int argc, char ** argv) {
	printf("Starting\n");
	if (argc != 2) {
		return ARGUMENT_ERROR;
	}
	uint64_t own_id = strtoul(argv[1], NULL, 0);
	uint64_t response_cnt = 0;
	uint64_t map_cnt = BRANCHING_FACTOR;
	uint64_t src_host_id = INVALID_ID;

	// Begin assembly core. Global state consists of response_cnt,
	// map_cnt, src_host_id, own_id
	lnic_set_enable(true);
	if (own_id == ROOT_ID) {
		// Send off the initial map messages if we're the root node
		for (uint64_t i = 0; i < map_cnt; i++) {
			lnic_write_word(MAP_ID); // Protocol id
			lnic_write_word(get_child_id(own_id, i)); // Destination id
			lnic_write_word(MAX_DEPTH); // Max depth
			lnic_write_word(1); // Current depth of the next layer
			lnic_write_word(own_id); // Source id
		}
	}
	while (true) {
		// Poll lnic until data is ready to read
		// printf("")
		while (!lnic_ready()) { // This can be a custom asm instruction for now

		}

		uint64_t message_type_id = lnic_read_word();
		uint64_t destination_id = lnic_read_word();
		if (destination_id != own_id) {
			return ID_MISMATCH;
		}
		if (message_type_id == MAP_ID) {
			// Handle map messages
			uint64_t max_depth = lnic_read_word();
			uint64_t cur_depth = lnic_read_word();
			if (cur_depth == max_depth - 1) {
				// Starting reduce phase
				lnic_write_word(REDUCE_ID);
				lnic_write_word(lnic_read_word()); // Send a response to the same host that sent this message
				return 0;
				// Send the reduce message
			} else {
				src_host_id = lnic_read_word();
				// For now, we'll always have four child boards
				for (uint64_t i = 0; i < map_cnt; i++) {
					// Send additional map messages
					lnic_write_word(MAP_ID);
					lnic_write_word(get_child_id(own_id, i));
					lnic_write_word(max_depth);
					lnic_write_word(cur_depth + 1);
					lnic_write_word(own_id);
				}
			}
		} else if (message_type_id == REDUCE_ID) {
			// Handle reduce messages
			response_cnt++;
			if (response_cnt == map_cnt) {
				// Send another reduce message if we have all of them gathered.
				if (src_host_id != INVALID_ID) {
					lnic_write_word(REDUCE_ID);
					lnic_write_word(src_host_id);
				} else {
					// Top level node, no need to send another reduce.
					return 0;
				}
			}

		} else {
			// Throw an error. Asm -- jump to test failure.
			return UNRECOGNIZED_MESSAGE;
		}
	}
	return 0;
}
