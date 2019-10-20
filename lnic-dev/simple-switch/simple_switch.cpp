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

void handle_connection(connection_t conn) {
	while (true) {
		uint8_t buffer[8];
		cout << "Connection waiting for data..." << endl;
		ssize_t actual_len = read(conn.connectionfd, buffer, 8);
		if (actual_len <= 0) {
			cerr << "Read failure at server." << endl;
			break;
		}
		for (int i = 0; i < 8; i++) {
			cout << buffer[i] << ", ";
		}
		cout << endl;
		connection_lock.lock();
		for (const auto& other_conn : all_connections) {
			// Just broadcast the data to all other clients
			// for now.
			if (other_conn.connectionfd == conn.connectionfd) {
				continue;
			}
			ssize_t written_len = write(other_conn.connectionfd,
					buffer, actual_len);
			if (written_len <= 0) {
				cerr << "Write failure at server." << endl;
			}
		}
		connection_lock.unlock();
	}
	connection_lock.lock();
	for (auto conn_iter = all_connections.begin(); conn_iter !=
			all_connections.end(); conn_iter++) {
		if ((*conn_iter).connectionfd == conn.connectionfd) {
			all_connections.erase(conn_iter);
			break;
		}
	}
	connection_lock.unlock();
}

int main() {
	int server_socket = start_server_socket(9000);
	if (server_socket < 0) {
		return -1;
	}
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
