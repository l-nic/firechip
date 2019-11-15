#include <unistd.h>
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

#include "loopback_tester.h"

const char * nic_config_data;

using namespace std;

void nic_t::populate_addr_maps() {
	ifstream id_addr_file;
	id_addr_file.open(ID_ADDR_FILENAME);
	string line;
	bool first_line = true;
	while (getline(id_addr_file, line)) {
		if (line.empty()) {
			continue;
		}
		if (first_line) {
			first_line = false;
			continue;
		}
		vector<string> split_line = split(line, ",");
		string fake_addr = split_line[0];
		string real_addr = split_line[1];
		uint16_t real_port = stoi(split_line[2]);
		_real_addr_map[fake_addr] = make_pair(real_addr, real_port);
		_fake_addr_map[make_pair(real_addr, real_port)] = fake_addr;
	}
	id_addr_file.close();
}

void nic_t::fake_to_real_addr(string& real_addr, uint16_t& real_port, string& fake_addr) {
	real_addr = _real_addr_map[fake_addr].first;
	real_port = _real_addr_map[fake_addr].second;
}

void nic_t::real_to_fake_addr(string& real_addr, uint16_t& real_port, string& fake_addr) {
	fake_addr = _fake_addr_map[make_pair(real_addr, real_port)];
}

uint64_t nic_t::get_port_id() {
	return 1; // TODO: Will eventually do a port id lookup based on the thread
}

nic_t::nic_t() {
	if (nic_config_data == nullptr) {
		return;
	}
	_own_ip_address(nic_config_data);
	populate_addr_maps();
	string real_addr;
	uint16_t real_port;
	fake_to_real_addr(real_addr, real_port, _own_ip_address);
	_server_socket = start_server_socket(real_port);
	if (_server_socket < 0) {
		cerr << "Failed to start server socket" << endl;
		exit(-1);
	}
	_write_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (_write_socket < 0) {
		printf("Failed to start write socket\n");
		exit(-1);
	}
	_all_threads.push_back(move(thread([this] () {
		listen_for_datagrams();
	})));
}

nic_t::~nic_t() {
	for (auto& t : _all_threads) {
		t.join();
	}
}

// Mimics a read of lnic read queue ready status register
uint64_t nic_t::num_messages_ready() {
	if (nic_config_data == nullptr) {
		return 0;
	}
	if (_enable) {
		return 0;
	}
	uint64_t port_id = get_port_id();
	_read_lock.lock();
	uint64_t num_read_messages = _num_read_messages;
	_read_lock.unlock();

	// Shouldn't be a problem to return this without a lock, since the queue is only
	// being drained by one thread, so there's no risk of getting a true here and
	// then having the queue actually be empty.
	return num_read_messages;
}

void nic_t::write_message() {
	// Build our payload for sending
	uint64_t datagram_len = (_write_message.words.size() / sizeof(uint64_t)) + sizeof(nic_t::header_data_t);
	uint8_t* datagram = new uint8_t[datagram_len];
	nic_t::header_data_t* header_data = (nic_t::header_data_t*)datagram;
	header_data->magic = LNIC_MAGIC;
	header_data->src_port_id = _write_message.per_message_data.src_port_id;
	header_data->dst_port_id = _write_message.per_message_data.dst_port_id;
	uint64_t* data_start = (uint64_t*)(datagram + sizeof(nic_t::header_data_t));
	for (size_t i = 0; i < _write_message.words.size(); i++) {
		data_start[i] = _write_message.words[i];
	}
	_write_message.words.clear();

	// Actually route the packet in the simulated system
	string fake_ip_address = _write_message.per_message_data.dst_ip_addr;
	string real_ip_addr;
	uint16_t real_udp_port;
	fake_to_real_addr(real_ip_addr, real_udp_port, fake_ip_address);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(real_udp_port);
	struct in_addr network_order_address;
    int ip_conversion_retval = inet_aton(real_ip_addr.c_str(), &network_order_address);
    if (ip_conversion_retval == 0) {
        printf("Ip conversion failure\n");
        exit(-1);
    }
    addr.sin_addr = network_order_address;

    // Send the packet
	ssize_t bytes_written = sendto(_write_socket, datagram, datagram_len, 0, (const struct sockaddr *)&addr, sizeof(addr));
	delete datagram;
	if (bytes_written <= 0) {
		printf("Error writing packet\n");
		exit(-1);
	}
}

void nic_t::handle_lnic_datagram(uint8_t* datagram, ssize_t datagram_len, string src_ip_address) {
	// We now have the raw data and the src ip address, same as we would
	// after deciphering the outer layers of a packet in a hardware implementation.
	// This implementation assumes that each packet contains either one full message
	// or part of one full message. Given that we're trying to minimize latency,
	// packing multiple messages into one packet (thus delaying the earlier ones)
	// would probably be a bad idea.

	// Everything in this section deals with lnic processing. This is fixed.
	if (datagram_len < sizeof(nic_t::header_data_t)) {
		return; // Drop packets that are too short
	}
	nic_t::header_data_t* lnic_header = (nic_t::header_data_t*)datagram;
	uint32_t lnic_magic = lnic_header->magic;
	if (lnic_magic != LNIC_MAGIC) {
		return; // Not an lnic packet, should never have been sent here
	}
	if (datagram_len - sizeof(nic_t::header_data_t) % sizeof(uint64_t) != 0) {
		// All lnic packets should have word-aligned data
		return;
	}
	uint64_t* data_ptr = (uint64_t*)(datagram + sizeof(nic_t::header_data_t));
	uint64_t word_len = (datagram_len - sizeof(nic_t::header_data_t)) / sizeof(uint64_t);
	uint64_t port_id = lnic_header->dst_port_id;
	nic_t::header_data_t* per_message_data = new nic_t::per_message_data;
	per_message_data->dst_port_id = lnic_header->dst_port_id;
	per_message_data->src_port_id = lnic_header->src_port_id;
	per_message_data->src_ip_addr = src_ip_addr;
	per_message_data->dst_ip_addr = _own_ip_address;

	// Everything in this section deals with pushing the received data into the
	// read queue for its destination thread. This section can be changed to
	// mimic implementing a transport protocol in hardware. Anything that gets
	// queued up still needs an end-marker and a 4-tuple, but there's no
	// requirement that that happen right away.
	_read_lock.lock();
	_read_queue.push({data_ptr, word_len, per_message_data});
	_num_read_messages++;
	_read_lock.unlock();
}

void nic_t::listen_for_datagrams() {
	uint8_t datagram[DATAGRAM_SIZE];
	while (true) {
		struct sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		ssize_t bytes_received = recvfrom(_server_socket, datagram, DATAGRAM_SIZE, MSG_WAITALL, (struct sockaddr *)&addr, &addr_len);

		if (bytes_received < 0) {
			cerr << "Datagram receive attempt failed."
				<< endl;
			break;
		}
		if (addr.sin_family != AF_INET) {
			cerr << "Server accepted non-internet datagram."
				<< endl;
			break;
		}
		uint16_t port = ntohs(addr.sin_port);
		char dst_cstring[INET_ADDRSTRLEN + 1];
		memset(dst_cstring, 0, INET_ADDRSTRLEN + 1);
		inet_ntop(AF_INET, &addr.sin_addr.s_addr, dst_cstring,
				INET_ADDRSTRLEN + 1);
		string ip_address(dst_cstring);
		string fake_ip_address;
		real_to_fake_addr(ip_address, port, fake_ip_address);
		handle_lnic_datagram(datagram, bytes_received, fake_ip_address);
	}
}

int nic_t::start_server_socket(uint16_t port) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
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

	return sockfd;
}

vector<string> nic_t::split(string input, string delim) {
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

int main(int argc, char ** argv) {
	if (argc != 2) {
		printf("Must supply an ip address\n");
		return -1;
	}
	nic_config_data = argv[1];
	nic_t nic;
	
	nic.write_message();
}
