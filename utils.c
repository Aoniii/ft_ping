#include "ping.h"
#include <arpa/inet.h>

unsigned short	calculate_checksum(void *b, int len) {
	unsigned short	*buf = b;
	unsigned int	sum = 0;
	unsigned short	result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;
	if (len == 1)
		sum += *(unsigned char *)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

void	setIPstr(struct addrinfo *res, char ip_str[][INET_ADDRSTRLEN]) {
	struct sockaddr_in	*ipv4 = (struct sockaddr_in *)res->ai_addr;
	inet_ntop(AF_INET, &(ipv4->sin_addr), *ip_str, INET_ADDRSTRLEN);
}

static const struct s_icmp_err g_icmp_errors[] = {
	{ICMP_DEST_UNREACH, ICMP_NET_UNREACH, "Destination Net Unreachable"},
	{ICMP_DEST_UNREACH, ICMP_HOST_UNREACH, "Destination Host Unreachable"},
	{ICMP_DEST_UNREACH, ICMP_PROT_UNREACH, "Destination Protocol Unreachable"},
	{ICMP_DEST_UNREACH, ICMP_PORT_UNREACH, "Destination Port Unreachable"},
	{ICMP_DEST_UNREACH, ICMP_FRAG_NEEDED, "Fragmentation needed and DF set"},
	{ICMP_DEST_UNREACH, ICMP_SR_FAILED, "Source Route Failed"},
	{ICMP_TIME_EXCEEDED, ICMP_EXC_TTL, "Time to live exceeded"},
	{ICMP_TIME_EXCEEDED, ICMP_EXC_FRAGTIME, "Frag reassembly time exceeded"},
	{ICMP_PARAMETERPROB, 0, "Parameter problem"},
	{ICMP_SOURCE_QUENCH, 0, "Source Quench (Congestion control)"}
};

const char	*get_icmp_error_msg(int type, int code) {
	for (size_t i = 0; i < sizeof(g_icmp_errors) / sizeof(g_icmp_errors[0]); i++) {
		if (g_icmp_errors[i].type == type && g_icmp_errors[i].code == code)
			return (g_icmp_errors[i].msg);
	}

	return ("Unknown ICMP Error");
}


