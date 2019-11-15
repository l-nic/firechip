#include <unistd.h>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;


#define CONFIG_TYPE 4

#define DATAGRAM_SIZE 1024

const uint64_t LNIC_MAGIC = 0xABCDEF12;

struct ConfigMessage {
	uint64_t message_type;
} __attribute__((packed));

struct header_data_t {
	uint64_t magic;
	uint64_t src_port_id;
	uint64_t dst_port_id;
} __attribute__((packed));

int main() {
	uint16_t port = 9001;
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("Server socket creation failed\n");
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
	uint8_t datagram[DATAGRAM_SIZE];
	while (true) {
		struct sockaddr_in src_addr;
		socklen_t addr_len = sizeof(src_addr);
		ssize_t bytes_received = recvfrom(sockfd, datagram, DATAGRAM_SIZE, MSG_WAITALL, (struct sockaddr *)&src_addr, &addr_len);

		if (bytes_received < 0) {
			cerr << "Datagram receive attempt failed."
				<< endl;
			break;
		}
		if (src_addr.sin_family != AF_INET) {
			cerr << "Server accepted non-internet datagram."
				<< endl;
			break;
		}
		uint16_t src_port = ntohs(src_addr.sin_port);
		char dst_cstring[INET_ADDRSTRLEN + 1];
		memset(dst_cstring, 0, INET_ADDRSTRLEN + 1);
		inet_ntop(AF_INET, &src_addr.sin_addr.s_addr, dst_cstring,
				INET_ADDRSTRLEN + 1);
		string ip_address(dst_cstring);
		cerr << "Received a datagram from ip address " << ip_address << " and udp port " << src_port << " of length " << bytes_received << endl;
		uint16_t response_port = ntohs(*(uint16_t*)datagram);
		cerr << "Datagram advertises a response port at udp " << response_port << endl;
		header_data_t* header = (header_data_t*)(datagram + sizeof(uint16_t));
		printf("Header magic number is %#lx, dst port is %d, src port is %d\n", header->magic, header->dst_port_id, header->src_port_id);
		uint64_t* words = (uint64_t*)(datagram + sizeof(uint16_t) + sizeof(header_data_t));
		uint64_t num_words = (bytes_received - sizeof(uint16_t) - sizeof(header_data_t)) / sizeof(uint64_t);
		printf("Received %d words\n", num_words);
		for (size_t i = 0; i < num_words; i++) {
			printf("Word %d: %#lx\n", i, words[i]);
		}
	}
	return 0;
}
