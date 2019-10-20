#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>

using namespace std;

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

int main() {
	int client_socket = start_client_socket("127.0.0.1", 9000);
	if (client_socket < 0) {
		return -1;
	}
	uint8_t buffer[8];
	memcpy(buffer, "abcdefg", 8);
	write(client_socket, buffer, 8);
	read(client_socket, buffer, 8);
	for (int i = 0; i < 8; i++) {
		cout << buffer[i] << ", ";
	}
	cout << endl;
	close(client_socket);
	return 0;
}
