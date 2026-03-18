#include "ping.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <arpa/inet.h>

void	ping(char **args, t_option *option) {
	int				sock_fd;
	struct addrinfo	hints;
	struct addrinfo	*res;

	if ((sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		perror("socket");
		return;
	}

	while (*args) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_RAW;

		if (getaddrinfo(*args, NULL, &hints, &res) != 0) {
			fprintf(stderr, "ft_ping: unknown host\n");
			return;
		}

		while (1) {
			printf("ping");
		}

		freeaddrinfo(res);
	}

	(void)args;
	(void)option;
}

