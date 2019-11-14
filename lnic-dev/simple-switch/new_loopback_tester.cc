#include <unistd.h>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>

using namespace std;


#define CONFIG_TYPE 4

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
	string real_ip_addr = "127.0.0.1";
	uint16_t real_udp_port = 9001;
	struct sockaddr_in addr;
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(real_udp_port);
	struct in_addr network_order_address;
	int ip_conversion_retval = inet_aton(real_ip_addr.c_str(), &network_order_address);
	if (ip_conversion_retval == 0) {
		printf("Ip conversion failure\n");
		return -1;
	}
	addr.sin_addr = network_order_address;
	header_data_t header;
	header.magic = LNIC_MAGIC;
	header.src_port_id = 1;
	header.dst_port_id = 1;

	ConfigMessage message;
	message.message_type = CONFIG_TYPE;
	uint64_t buffer_size = sizeof(header_data_t) + sizeof(ConfigMessage);
	uint8_t* buffer = new uint8_t[buffer_size];
	ssize_t bytes_written = sendto(sockfd, buffer, buffer_size, 0, (const struct sockaddr*)&addr, sizeof(addr));
	delete [] buffer;
	if (bytes_written <= 0) {
		printf("Write error sending datagram\n");
		return -1;
	}
	return 0;
}
