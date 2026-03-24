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

const char	*get_icmp_error_msg(int type, int code) {
	for (size_t i = 0; i < sizeof(g_icmp_errors) / sizeof(g_icmp_errors[0]); i++) {
		if (g_icmp_errors[i].type == type && g_icmp_errors[i].code == code)
			return (g_icmp_errors[i].msg);
	}

	return ("Unknow ICMP Error");
}


