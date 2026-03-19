#include "ping.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

static unsigned short calculate_checksum(void *b, int len) {
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

void	ping(char **args, t_option *option) {
	int				sock_fd;
	struct addrinfo	hints;
	struct addrinfo	*res;
	struct icmphdr	*icmp;

	//var temp
	int	payload_size = 56; //a changer c'est le -s de base c'est 56
	int	size = sizeof(struct icmphdr) + payload_size;

	// 1. Socket
	if ((sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		perror("socket");
		return;
	}

	// 2. DNS
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;

	if (getaddrinfo(*args, NULL, &hints, &res) != 0) {
		fprintf(stderr, "ft_ping: unknown host\n");
		return;
	}

	// 3. Ping
	char	packet[IP_MAXPACKET];
	char	recv_buf[IP_MAXPACKET];
	int		sequence = 0;

	// recup en str l'ip
	char				ip_str[INET_ADDRSTRLEN];
	struct sockaddr_in	*ipv4 = (struct sockaddr_in *)res->ai_addr;
	inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, INET_ADDRSTRLEN);

	printf("PING %s (%s): %d data bytes\n", args[0], ip_str, payload_size);

	icmp = (struct icmphdr *)packet;
	while (1) {
		memset(packet, 0, size);

		icmp->type = ICMP_ECHO;
		icmp->code = 0;
		icmp->un.echo.id = getpid();
		icmp->un.echo.sequence = sequence++;
		icmp->checksum = calculate_checksum(packet, size);

		// 1e partie pour le temps
		struct timeval start, end;
		gettimeofday(&start, NULL);

		if (sendto(sock_fd, packet, size, 0, res->ai_addr, res->ai_addrlen) <= 0) {
			perror("sendto");
			return;
		}

		struct sockaddr_in from;
		socklen_t from_len = sizeof(from);
		if (recvfrom(sock_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&from, &from_len) > 0) {
			// 2e partie pour le temps
			gettimeofday(&end, NULL);
			double diff = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;

			// ttl
			struct ip		*ip_hdr = (struct ip *)recv_buf;
			int				ttl = ip_hdr->ip_ttl;

			printf(
					"%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
					size,
					inet_ntoa(from.sin_addr),
					sequence - 1,
					ttl,
					diff
			);
		}

		sleep(1);
	}

	freeaddrinfo(res);

	(void)option;
}

