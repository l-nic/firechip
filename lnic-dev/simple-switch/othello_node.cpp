// For now, have a fixed allocation of nodes permitting the full branching factor
// to be created

// Nodes further down the tree will serve as servers for nodes up the tree
// They'll be automatically started if they can't be contacted.
// All nodes have a tuple id of layer, index in layer


// OR, since everything will eventually need to be doable by an assembly program:
// The same node code should ideally also work for a grid or for a neural net,
// with just the connections rewired.

// We can assume that the assembly code can pre-read its own id and use that
// to calculate who it's connected to. In the sim, we can have the NIC
// initialization (which is in C++) do this. In practice, this would be
// a configuration step for the machine that is starting the nanoservice.
// We would need a way to inform the NIC of the transport protocol (TCP),
// application protocol (Othello), and application-impacting transport
// header fields (i.e. source and dest. IP/port pairs -- the application
// needs these to figure out where everything is supposed to go, but
// they don't need to be dealt with in every message.)

// Once we know who we're connected to and who we are, the process becomes a lot easier.
// For othello, one node will start by blasting out map messages to its neighbors.
// All subsequent nodes will continue blasting the messages out to their lower neighbors
// in the tree.
// Because we're not just sending the same message to every neighbor, we'll need to
// re-transmit the message for each different neighbor. Because the destination ip/port
// is not a part of the primary message, we'll need a special type of control instruction
// to provide that (or to change the message type to tcp/ip).
// We'll also need to change the message type from map to reduce at some point for othello.

// For reading the messages, we'll need a really quick way to get the message type
// as soon as a message is ready. It can be assumed that this message type will not
// change while we're reading/processing the message. The read and write message
// types can, naturally, be different.

// So, overall:
// We need to set up and configure the connections at the beginning.
// Then we need to figure out out to handle message types and route messages
// to their correct destinations. (It could be possible to shove some of this
// destination information into the application message, but to a certain
// extent, it doesn't matter -- the destination info will need to be accounted
// for in some sort of p4 message type somewhere, it's just a question of whether
// that's in the application message or in a separate control message.)
// (Look at control planes.)

// Configuration software will also need to define different groups for
// determining which socket everything gets forwarded onto. (Or we
// could use the id's to do the calculation.)

// Have the config format be: ID, list of server id's to connect to
// These will be passed in as arguments by a python script that starts all
// servers in quick succession. Each server will delay for a second in
// between starting its listen and attempting new connections to allow
// time for all servers to be brought up.
// Assembly servers will be started by the same script.
// All servers will have access to a hosts file mapping id to ip/port pair.
// When a client connects to a server, it sends a custom message with its
// own ip/port pair. As this is a message, the assembly servers will send
// it with the assembly interface.

// Possible change: All outbound data goes from client --> server
// This should really be datagrams...

// The queue is going to need to know how to handle different types of messages.

#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <iostream>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <chrono>
#include <map>
#include <fstream>
#include <queue>


using namespace std;

int start_server_socket(uint16_t port);
const int CONN_BACKLOG = 32;
vector<thread> all_threads;

struct connection_t {
	int connectionfd;
	uint16_t port;
	string ip_address;
};
mutex connection_lock; // TODO: This is probably really slow.
vector<connection_t> all_connections; // TODO: Get rid of global variables.

const uint64_t INVALID_ID = 0;
const uint64_t MAP_ID = 1; // This is very wasteful reading these id's in all the time.
// We probably want to be able to configure one message type and then have it stick around
// for a while.
const uint64_t REDUCE_ID = 2;

// Host id's, not message id's
const uint64_t ROOT_ID = 1;

// Might want to see how feasible it is to use protobufs here.
struct Message {
	uint64_t message_type_id = INVALID_ID;
	uint64_t destination_id = INVALID_ID;
};

struct MapMessage : Message {
	uint64_t max_depth = 3;
	uint64_t cur_depth = 0;
	uint64_t src_host_id = INVALID_ID;
};

struct ReduceMessage : Message {

};

uint64_t glob_map_cnt = 4; // TODO: Get rid of this

struct MessageState {
	uint64_t response_cnt = 0;
	uint64_t map_cnt = glob_map_cnt;
	uint64_t src_host_id = INVALID_ID;
};

mutex read_lock;
queue<Message*> read_queue;
queue<Message*> write_queue;

map<uint64_t, uint64_t> message_ids;

void handle_connection(connection_t conn) {
	int message_index = 0;
	while (true) {
		// This scheme requires sending the message type id first
		uint64_t message_type_id;
		cout << "Connection waiting for data..." << endl;
		ssize_t actual_len = read(conn.connectionfd, &message_type_id, 8);
		if (actual_len <= 0) {
			cerr << "Read failure at server in header" << endl;
			break;
		}
		cerr << "Message type id is " << message_type_id << endl;
		uint64_t message_size = message_ids[message_type_id];
		uint64_t* buffer = new uint64_t[message_size];
		buffer[0] = message_type_id;
		cerr << "Message size is " << message_size << endl;
		actual_len = read(conn.connectionfd, buffer + 1, (message_size-1)*8);
		if (actual_len <= 0) {
			cerr << "Read failure at server." << endl;
			break;
		}
		//cerr << "Message "
		read_lock.lock();
		read_queue.push(reinterpret_cast<Message*>(buffer));
		read_lock.unlock();
	
	}
}

vector<string> split(string input, string delim) {
	vector<string> to_return;
	while (true) {
		size_t delim_pos = input.find(delim);
		if (delim_pos == string::npos) {
			break;
		}
		to_return.push_back(input.substr(0, delim_pos));
		input = input.substr(delim_pos + 1);
	}
	to_return.push_back(input);
	return to_return;
}

map<uint64_t, connection_t> get_id_addr_map() {
	ifstream id_addr_file;
	id_addr_file.open("id_addr.txt");
	string line;
	bool first_line = true;
	map<uint64_t, connection_t> id_addr_map;
	while (getline(id_addr_file, line)) {
		if (first_line) {
			first_line = false;
			continue;
		}
		vector<string> split_line = split(line, ",");
		uint64_t id = stol(split_line[0]);
		string ip_addr = split_line[1];
		uint16_t port = stoi(split_line[2]);
		id_addr_map[id] = {-1, port, ip_addr};
	}
	id_addr_file.close();
	return id_addr_map;
}

int start_client_socket(string ip_address, uint16_t port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return -1;
	}
	struct in_addr network_order_address;
	int ip_conversion_retval = inet_aton(ip_address.c_str(), &network_order_address);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = network_order_address;
	int connect_retval = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
	if (connect_retval < 0) {
		close(sockfd);
		return -1;
	}
	return sockfd;
}

// Mimics a read of lnic read queue ready status register
bool lnic_ready() {
	read_lock.lock();
	bool ready = !read_queue.empty();
	read_lock.unlock();
	// Shouldn't be a problem to read this without a lock, since the queue is only
	// being drained by one thread, so there's no risk of getting a true here and
	// then having the queue actually be empty.
	return ready;
}

// Returns an 8-byte word read from the front of the lnic read queue.
// Only safe to call if lnic_read_ready returned true and no context switch
// occurred in between.
Message* current_message = nullptr;
uint64_t message_index = 0;

uint64_t lnic_read_word() {
	if (current_message && message_index < message_ids[current_message->message_type_id]) {
		// We have a current message and we're not done with it yet
		uint64_t next_word = reinterpret_cast<uint64_t*>(current_message)[message_index];
		message_index++;
		return next_word;
	}

	// We need to read a new message
	read_lock.lock();
	if (read_queue.empty()) {
		cerr << "Tried to read from an empty lnic queue!" << endl;
		exit(-1);
	}
	if (current_message) {
		delete [] current_message; // TODO: Check that this is the right kind of delete.
		// (It was created as an array, so probably.)
	}
	message_index = 0;
	current_message = read_queue.front();
	read_queue.pop();
	uint64_t next_word = reinterpret_cast<uint64_t*>(current_message)[message_index];
	message_index++;
	read_lock.unlock();
	return next_word;
}

// TODO: Right now, the application message type is defined within the message itself. Make
// sure we actually want this. Each message also has to include its destination, or the
// sending logic that processes the write queue won't know where to send it.

Message* current_write_message = nullptr;
uint64_t write_message_index = 0;
void lnic_write_word(uint64_t word) {
	if (write_message_index == 0) {
		// Start of a new message, set its type
		uint64_t message_type_id = word;
		uint64_t message_size = message_ids[message_type_id];
		current_write_message = reinterpret_cast<Message*>(new uint64_t[message_size]);
		current_write_message->message_type_id = message_type_id;
		write_message_index++;
	} else {
		// Other data in the message, fill it in
		reinterpret_cast<uint64_t*>(current_write_message)[write_message_index] = word;
		write_message_index++;
	}

	if (write_message_index == message_ids[current_write_message->message_type_id]) {
		// Message is complete, write it out to the queue
		write_queue.push(current_write_message); // No locks on this since it's only used in one thread in run_task
		write_message_index = 0;
	}
}

void flush_write_queue(map<uint64_t, connection_t> id_addr_map) {
	// This function will for now require that the first non-message-id field of every message specifies its
	// destination id. This will be easier to set up than supporting out-of-band metadata (i.e. tcp/ip
	// or udp setup info sent via separate control instructions).
	while (!write_queue.empty()) {
		Message* to_send = write_queue.front();
		write_queue.pop();
		cerr << "Writing message with type " << to_send->message_type_id << " and dest " << to_send->destination_id << endl;
		int write_fd = id_addr_map[to_send->destination_id].connectionfd;
		ssize_t written_len = write(write_fd, to_send, message_ids[to_send->message_type_id]*8);
		if (written_len <= 0) {
			cerr << "Error writing to destination " << to_send->destination_id << endl;
		}
		delete [] to_send;
	}
}

uint64_t get_child_id(uint64_t own_id, uint64_t i) {
	return (glob_map_cnt * own_id) - (glob_map_cnt - 2) + i;
}

void run_task(uint64_t own_id, vector<uint64_t> server_ids, map<uint64_t, connection_t> id_addr_map) {
	this_thread::sleep_for(chrono::milliseconds(3000));
	for (uint64_t server_id : server_ids) {
		// Connect to each listed server id
		int sockfd = start_client_socket(id_addr_map[server_id].ip_address, id_addr_map[server_id].port);
		if (sockfd < 0) {
			cerr << "Error connecting to server at ip address " << id_addr_map[server_id].ip_address << " and port " << id_addr_map[server_id].port << endl;
			continue;
		}
		id_addr_map[server_id].connectionfd = sockfd;
	}

	// Now that we have all of the grunt work set up, we bascially sit here polling to see if any messages have arrived.
	// If they have, we'll process them and send them on.
	// Everything from here on out (in this function) needs to be translated into assembly.
	MessageState state;
	if (own_id == ROOT_ID) {
		MapMessage map_message;
		// Send off the initial map messages if we're the root node
		for (uint64_t i = 0; i < state.map_cnt; i++) {
			lnic_write_word(MAP_ID);
			lnic_write_word(get_child_id(own_id, i));
			lnic_write_word(map_message.max_depth); // The struct defines the defaults
			lnic_write_word(map_message.cur_depth);
			lnic_write_word(own_id);
		}
	}
	while (true) {
		// Poll lnic until data is ready to read
		while (!lnic_ready()) { // This can be a custom asm instruction for now
			this_thread::sleep_for(chrono::milliseconds(10)); // This can just be a very long loop in asm
		}

		uint64_t message_type_id = lnic_read_word();
		uint64_t destination_id = lnic_read_word();
		if (destination_id != own_id) {
			cerr << "Destination id " << destination_id << " does not match own id " << own_id << endl;
		}
		if (message_type_id == MAP_ID) {
			// Handle map messages
			uint64_t max_depth = lnic_read_word();
			uint64_t cur_depth = lnic_read_word();
			if (cur_depth == max_depth - 1) {
				// Starting reduce phase
				cerr << "Node " << own_id << " starting reduce phase." << endl;
				lnic_write_word(REDUCE_ID);
				lnic_write_word(lnic_read_word()); // Send a response to the same host that sent this message

				// Send the reduce message
			} else {
				state.src_host_id = lnic_read_word();
				// For now, we'll always have four child boards
				for (uint64_t i = 0; i < state.map_cnt; i++) {
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
			cerr << "Handling reduce message." << endl;
			state.response_cnt++;
			if (state.response_cnt == state.map_cnt) {
				// Send another reduce message if we have all of them gathered.
				if (state.src_host_id != INVALID_ID) {
					lnic_write_word(REDUCE_ID);
					lnic_write_word(state.src_host_id);
				} else {
					// Top level node, no need to send another reduce.
					cerr << "Top level node received last reduce message." << endl;
				}
				break;
			}

		} else {
			// Throw an error. Asm -- jump to test failure.
			cerr << "Unrecognized message type id " << message_type_id << endl;
		}
		break;

		flush_write_queue(id_addr_map); // Don't translate to asm, this will be handled by hardware
		// In the simulator, we might have to get rid of the write queue and just write out
		// each message immediately as it's built.
	}
}

int main(int argc, char** argv) {
	if (argc < 2) {
		cerr << "Own ID must be defined" << endl;
		return -1;
	}
	uint64_t own_id = strtol(argv[1], NULL, 0);
	vector<uint64_t> server_ids;
	for (int i = 2; i < argc; i++) {
		server_ids.push_back(strtol(argv[i], NULL, 0));
	}
	map<uint64_t, connection_t> id_addr_map = get_id_addr_map();
	message_ids[MAP_ID] = sizeof(MapMessage) / 8;
	message_ids[REDUCE_ID] = sizeof(ReduceMessage) / 8;


	int server_socket = start_server_socket(id_addr_map[own_id].port);
	if (server_socket < 0) {
		return -1;
	}
	all_threads.push_back(move(thread([own_id, server_ids, id_addr_map] () {
		run_task(own_id, server_ids, id_addr_map);
	})));
	while (true) {
		struct sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		cout << "Waiting for connection..." << endl;
		int connectionfd = accept(server_socket, (struct sockaddr *)
				&addr, &addr_len);
		if (connectionfd < 0) {
			cerr << "Server connection accept attempt failed."
				<< endl;
			continue;
		}
		if (addr.sin_family != AF_INET) {
			cerr << "Server accepted non-internet connection."
				<< endl;
			continue;
		}
		uint16_t port = ntohs(addr.sin_port);
		char dst_cstring[INET_ADDRSTRLEN + 1];
		memset(dst_cstring, 0, INET_ADDRSTRLEN + 1);
		inet_ntop(AF_INET, &addr.sin_addr.s_addr, dst_cstring,
				INET_ADDRSTRLEN + 1);
		string ip_address(dst_cstring);
		connection_t conn;
		conn.connectionfd = connectionfd;
		conn.port = port;
		conn.ip_address = ip_address;
		all_connections.push_back(conn);
		all_threads.push_back(move(thread([conn] () {
						handle_connection(conn); })));
	}
	for (auto& t : all_threads) {
		t.join();
	}
	return 0;
}

int start_server_socket(uint16_t port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		cerr << "Server socket creation failed." << endl;
		return -1;
	}
	int sockopt_enable = 1;
	int setopt_retval = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
			&sockopt_enable, sizeof(int));
	if (setopt_retval < 0) {
		cerr << "Server socket option setting failed." << endl;
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int bind_retval = bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
	if (bind_retval < 0) {
		cerr << "Server socket bind failure." << endl;
		return -1;
	}

	int listen_retval = listen(sockfd, CONN_BACKLOG);
	if (listen_retval < 0) {
		cerr << "Server socket listen failure." << endl;
		return -1;
	}
	return sockfd;
}
