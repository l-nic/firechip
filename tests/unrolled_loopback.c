#include "mmio.h"
#include "nic.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ETH_MAX_WORDS 190
#define NET_IP_ALIGN 2
#define ETH_HEADER_SIZE 14
#define MAC_ADDR_SIZE 6
#define IP_ADDR_SIZE 4

#define IPV4_ETHTYPE 0x0800
#define ARP_ETHTYPE 0x0806
#define ICMP_PROT 1
#define ECHO_REPLY 0
#define ECHO_REQUEST 8
#define ARP_REQUEST 1
#define ARP_REPLY 2
#define HTYPE_ETH 1

#define UDP_PROT 17
#define IP_MAX_STR_LEN 16
#define ICMP_DESTINATION_UNREACHABLE 3

register volatile uint64_t cycle_start asm("s1");
register volatile uint64_t cycle_end asm("s3");
register volatile uint64_t inst_start asm("s2");
register volatile uint64_t inst_end asm("s4");
#define PROFILE_START asm volatile("rdinstret %0" : "=r"(inst_start)); asm volatile("rdcycle %0" : "=r"(cycle_start));
#define PROFILE_STOP asm volatile("rdcycle %0" : "=r"(cycle_end)); asm volatile("rdinstret %0" : "=r"(inst_end));
#define PROFILE_DUMP printf("%u,%u", inst_end - inst_start, cycle_end - cycle_start);

//#define PROFILE_START_RESERVE asm volatile("rdinstret s1" : : "s1")

static inline uint16_t ntohs(uint16_t nint)
{
	return ((nint & 0xff) << 8) | ((nint >> 8) & 0xff);
}

static inline uint16_t htons(uint16_t nint)
{
	return ntohs(nint);
}

static inline void parse_ip_addr(char* result, uint32_t input) {
	uint8_t first = input & 0xff;
	uint8_t second = (input >> 8) & 0xff;
	uint8_t third = (input >> 16) & 0xff;
	uint8_t fourth = (input >> 24) & 0xff;
	sprintf(result, "%d.%d.%d.%d", first, second, third, fourth);
}

struct eth_header {
	uint8_t padding[NET_IP_ALIGN];
	uint8_t dst_mac[MAC_ADDR_SIZE];
	uint8_t src_mac[MAC_ADDR_SIZE];
	uint16_t ethtype;
};

struct arp_header {
	uint16_t htype;
	uint16_t ptype;
	uint8_t hlen;
	uint8_t plen;
	uint16_t oper;
	uint8_t sha[MAC_ADDR_SIZE];
	uint8_t spa[IP_ADDR_SIZE];
	uint8_t tha[MAC_ADDR_SIZE];
	uint8_t tpa[IP_ADDR_SIZE];
};

struct ipv4_header {
	uint8_t ver_ihl;
	uint8_t dscp_ecn;
	uint16_t length;
	uint16_t ident;
	uint16_t flags_frag_off;
	uint8_t ttl;
	uint8_t prot;
	uint16_t cksum;
	uint32_t src_addr;
	uint32_t dst_addr;
};

struct icmp_header {
	uint8_t type;
	uint8_t code;
	uint16_t cksum;
	uint32_t rest;
};

struct udp_header {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t length;
	uint16_t checksum;
};

struct lnic_header_t {
	uint64_t magic;
	uint64_t src_port;
	uint64_t dst_port;
} __attribute__((packed));

struct basic_message_t {
	uint64_t word;
} __attribute__((packed));

static int checksum(uint16_t *data, int len)
{
	int i;
	uint32_t sum = 0;

	for (i = 0; i < len; i++)
		sum += ntohs(data[i]);

	while ((sum >> 16) != 0)
		sum = (sum & 0xffff) + (sum >> 16);

	sum = ~sum & 0xffff;

	return sum;
}

#define ceil_div(n, d) (((n) - 1) / (d) + 1)

static int process_arp(void *buf, uint8_t *mac)
{
	struct eth_header *eth = buf;
	struct arp_header *arp;
	size_t size = ETH_HEADER_SIZE + sizeof(*arp);
	uint8_t tmp_addr[IP_ADDR_SIZE];

	// Verify arp packet
	arp = buf + sizeof(*eth);
	if (ntohs(arp->oper) != ARP_REQUEST) {
		printf("Wrong arp operation: %d\n", ntohs(arp->oper));
		return -1;
	}

	if (ntohs(arp->htype) != HTYPE_ETH) {
		printf("Wrong ARP HTYPE\n");
		return -1;
	}

	if (ntohs(arp->ptype) != IPV4_ETHTYPE) {
		printf("Wrong ARP PTYPE\n");
		return -1;
	}

	if (arp->hlen != 6) {
		printf("Wrong ARP HLEN: %d\n", arp->hlen);
		return -1;
	}

	if (arp->plen != 4) {
		printf("Wrong ARP PLEN: %d\n", arp->plen);
		return -1;
	}

	// Make the source the destination, and add our mac address
	memcpy(eth->dst_mac, eth->src_mac, MAC_ADDR_SIZE);
	memcpy(eth->src_mac, mac, MAC_ADDR_SIZE);

	// create ARP reply
	arp->oper = htons(ARP_REPLY);

	// Make tha the sha, and fill in sha with actual mac address
	memcpy(arp->tha, arp->sha, MAC_ADDR_SIZE);
	memcpy(arp->sha, mac, MAC_ADDR_SIZE);

	// Swap spa and tpa in arp packet
	memcpy(tmp_addr, arp->tpa, IP_ADDR_SIZE);
	memcpy(arp->tpa, arp->spa, IP_ADDR_SIZE);
	memcpy(arp->spa, tmp_addr, IP_ADDR_SIZE);

	size = ceil_div(size + NET_IP_ALIGN, 8) * 8;
	nic_send(buf, size);

	return 0;
}

static int process_icmp(void *buf, uint8_t *mac) {
	struct eth_header *eth = buf;
	struct ipv4_header *ipv4;
	struct icmp_header *icmp;
	int ihl, icmp_size;
	ssize_t size;
	uint32_t tmp_addr;

	// verify IPv4
	ipv4 = buf + sizeof(*eth);
	ihl = ipv4->ver_ihl & 0xf;

	if (checksum((uint16_t *) ipv4, ihl << 1) != 0) {
		printf("Bad IP header checksum %04x\n", ipv4->cksum);
		return -1;
	}

	// verify ICMP
	icmp = (buf + sizeof(*eth) + (ihl << 2));

	if (icmp->type != ECHO_REQUEST) {
		if (icmp->code == ICMP_DESTINATION_UNREACHABLE) {
			printf("Destination host unreachable\n");
			return 0;
		}
		printf("Wrong ICMP type %d\n", icmp->type);
		return -1;
	}

	if (icmp->code != 0) {
		printf("Wrong ICMP code %d\n", icmp->code);
		return -1;
	}

	icmp_size = ntohs(ipv4->length) - (ihl << 2);
	if (checksum((uint16_t *) icmp, icmp_size >> 1) != 0) {
		printf("Bad ICMP checksum %04x\n", icmp->cksum);
		return -1;
	}

	// Set the destination and source MACs
	memcpy(eth->dst_mac, eth->src_mac, MAC_ADDR_SIZE);
	memcpy(eth->src_mac, mac, MAC_ADDR_SIZE);

	// Swap the source and destination IP addresses
	tmp_addr = ipv4->dst_addr;
	ipv4->dst_addr = ipv4->src_addr;
	ipv4->src_addr = tmp_addr;

	// compute the IPv4 header checksum
	ipv4->cksum = 0;
	ipv4->cksum = htons(checksum((uint16_t *) ipv4, ihl << 1));

	// set the ICMP type to reply and compute checksum
	icmp->cksum = 0;
	icmp->type = ECHO_REPLY;
	icmp->cksum = htons(checksum((uint16_t *) icmp, icmp_size >> 1));
	size = ntohs(ipv4->length) + ETH_HEADER_SIZE;

	size = ceil_div(size + NET_IP_ALIGN, 8) * 8;
	nic_send(buf, size);

	return 0;
}

static void print_message(struct ipv4_header* ipv4, struct udp_header* udp, uint16_t* reply_port_addr, struct lnic_header_t* lnic_header, struct basic_message_t* message_data, uint64_t message_size_words) {
	printf("START OF MESSAGE--------------\n");
	char src_addr[IP_MAX_STR_LEN];
	char dst_addr[IP_MAX_STR_LEN];
	parse_ip_addr(src_addr, ipv4->src_addr);
	parse_ip_addr(dst_addr, ipv4->dst_addr);
	printf("IP layer: src addr %s, dst addr %s\n", src_addr, dst_addr);
	printf("UDP layer: src port %d, dst port %d, datagram size in bytes %d\n", ntohs(udp->src_port), ntohs(udp->dst_port), ntohs(udp->length));
	printf("UDP reply layer: reply port %d\n", ntohs(*reply_port_addr));
	printf("LNIC header layer: magic number 0x%#lx, src port %d, dst port %d\n", lnic_header->magic, lnic_header->src_port, lnic_header->dst_port);
	printf("LNIC message length in words: %d\n", message_size_words);
	printf("LNIC message data by word:\n");
	for (size_t i = 0; i < message_size_words; i++) {
		printf("    %d: 0x%#lx\n", i, message_data[i].word);
	}
	printf("END OF MESSAGE--------------------\n\n");
}

static int process_udp(void *buf, uint8_t *mac) {
	struct eth_header *eth = buf;
	// verify IPv4
	struct ipv4_header* ipv4 = buf + sizeof(*eth);
	int ihl = ipv4->ver_ihl & 0xf;

	if (checksum((uint16_t *) ipv4, ihl << 1) != 0) {
		printf("Bad IP header checksum %04x\n", ipv4->cksum);
		return -1;
	}

	// Parse UDP, reply port, LNIC header, message start pointer, and message size
	struct udp_header* udp = ((char*)ipv4 + (ihl << 2));
	uint16_t* reply_port_addr = ((char*)udp + sizeof(*udp));
	struct lnic_header_t* lnic_header = ((char*)reply_port_addr + sizeof(uint16_t));
	struct basic_message_t* message_data = ((char*)lnic_header + sizeof(struct lnic_header_t));
	uint64_t message_size_words = (ntohs(udp->length) - sizeof(*udp) - sizeof(uint16_t) - sizeof(*lnic_header)) / sizeof(uint64_t);
	
	print_message(ipv4, udp, reply_port_addr, lnic_header, message_data, message_size_words);

	volatile uint64_t word0 = message_data[0].word;
	volatile uint64_t word1 = message_data[1].word;
	volatile uint64_t word2 = message_data[2].word;

	PROFILE_START;
	word0++;
	word1++;
	word2++;
	PROFILE_STOP;

	message_data[0].word = word0;
	message_data[1].word = word1;
	message_data[2].word = word2;

	printf("COPY,");
	PROFILE_DUMP;
	printf("\n");

	// Build the outer headers to route the packet back
	// Set the destination and source MACs
	memcpy(eth->dst_mac, eth->src_mac, MAC_ADDR_SIZE);
	memcpy(eth->src_mac, mac, MAC_ADDR_SIZE);

	// Swap the source and destination IP addresses
	uint32_t tmp_addr = ipv4->dst_addr;
	ipv4->dst_addr = ipv4->src_addr;
	ipv4->src_addr = tmp_addr;

	// compute the IPv4 header checksum
	ipv4->cksum = 0;
	ipv4->cksum = htons(checksum((uint16_t *) ipv4, ihl << 1));

	// Set the UDP header fields that need to change
	udp->src_port = htons(9000);
	udp->dst_port = *reply_port_addr;
	udp->checksum = 0;

	// Set the new reply port
	*reply_port_addr = htons(9000);

	ssize_t size = ntohs(ipv4->length) + ETH_HEADER_SIZE;

	size = ceil_div(size + NET_IP_ALIGN, 8) * 8;

	PROFILE_START;
	nic_send(buf, size);
	PROFILE_STOP;
	printf("SEND,");
	PROFILE_DUMP;
	printf("\n");
	return 0;
}

static int process_ipv4(void *buf, uint8_t *mac)
{
	struct eth_header *eth = buf;
	struct ipv4_header *ipv4;
	struct icmp_header *icmp;
	int ihl, icmp_size;
	ssize_t size;
	uint32_t tmp_addr;

	// verify IPv4
	ipv4 = buf + sizeof(*eth);
	ihl = ipv4->ver_ihl & 0xf;

	if (checksum((uint16_t *) ipv4, ihl << 1) != 0) {
		printf("Bad IP header checksum %04x\n", ipv4->cksum);
		return -1;
	}

	if (ipv4->prot == ICMP_PROT) {
		return process_icmp(buf, mac);
	} else if (ipv4->prot == UDP_PROT) {
		return process_udp(buf, mac);
	} else {
		printf("Wrong IP protocol %d\n", ipv4->prot);
		return -1;
	}
}

static int process_packet(void *buf, uint8_t *mac)
{
	struct eth_header *eth;

	// read the ICMP request
	nic_recv(buf);
	eth = buf;
	printf("Got packet: [ethtype=%04x]\n", ntohs(eth->ethtype));
	// Check ethernet type
	switch (ntohs(eth->ethtype)) {
	case IPV4_ETHTYPE:
		return process_ipv4(buf, mac);
	case ARP_ETHTYPE:
		return process_arp(buf, mac);
	default:
		printf("Wrong ethtype %x\n", ntohs(eth->ethtype));
		return -1;
	}
}

uint64_t buffer[ETH_MAX_WORDS];

int main(void)
{
	uint64_t macaddr_long;
	uint8_t *macaddr;

	macaddr_long = nic_macaddr();
	macaddr = (uint8_t *) &macaddr_long;

	printf("macaddr - %02x", macaddr[0]);
	for (int i = 1; i < MAC_ADDR_SIZE; i++)
		printf(":%02x", macaddr[i]);
	printf("\n");

	for (;;) {
		if (process_packet(buffer, macaddr))
			return -1;
	}

	return 0;
}
