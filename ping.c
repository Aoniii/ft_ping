#include "ping.h"
#include <bits/types/struct_timeval.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

static t_error	init(int *sock_fd, struct addrinfo *hints, struct addrinfo **res, char *args, t_stats *stats) {
	// 1. Socket creation
	if ((*sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		perror("socket");
		return (ERROR);
	}

	// 2. Timeout configuration
	struct timeval	tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(*sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("setsockopt");
		return (ERROR);
	}

	// 3. DNS
	memset(hints, 0, sizeof(*hints));
	(*hints).ai_family = AF_INET;
	(*hints).ai_socktype = SOCK_RAW;

	if (getaddrinfo(args, NULL, hints, res) != 0) {
		fprintf(stderr, "ft_ping: unknown host\n");
		return (ERROR);
	}

	// 4. Init stats
	stats->addr = args;
	stats->min = 1e9;
	stats->max = 0;
	stats->sum = 0;
	stats->sum_sq = 0;
	gettimeofday(&stats->start, NULL);

	return (SUCCESS);
}

void	ping(char **args, t_data data) {
	int				sock_fd;
	struct addrinfo	hints;
	struct addrinfo	*res;
	struct icmphdr	*icmp;

	int	index = 0;
	int	payload_size = 56; //a changer c'est le -s de base c'est 56
	int	total_size = sizeof(struct icmphdr) + payload_size;

	while (args[index] && g_running) {
		t_stats	stats;
		char	packet[IP_MAXPACKET];
		char	recv_buf[IP_MAXPACKET];
		char	ip_str[INET_ADDRSTRLEN];
		int		sequence = 0;

		sock_fd = -1;
		res = NULL;
		memset(&stats, 0, sizeof(t_stats));

		if (init(&sock_fd, &hints, &res, args[index], &stats) != SUCCESS)
			break;

		signal(SIGINT, sig_handler);
		setIPstr(res, &ip_str);
		icmp = (struct icmphdr *)packet;

		printf("PING %s (%s): %d data bytes", args[index], ip_str, payload_size);
		if (data.verbose)
			printf(", id 0x%04x = %u", getpid() & 0xFFFF, getpid() & 0xFFFF);
		printf("\n");

		while (g_running) {
			// 1. Setup icmp 
			memset(packet, 0, total_size);
			icmp->type = ICMP_ECHO;
			icmp->code = 0;
			icmp->un.echo.id = getpid();
			icmp->un.echo.sequence = sequence++;
			icmp->checksum = calculate_checksum(packet, total_size);

			// 2. Set send time
			struct timeval start;
			gettimeofday(&start, NULL);

			// 3. Send packet
			stats.transmitted++;
			if (sendto(sock_fd, packet, total_size, 0, res->ai_addr, res->ai_addrlen) <= 0) {
				perror("sedto");
				return;
			}

			// 4. Receive packet
			struct timeval		end;
			struct sockaddr_in	from;
			socklen_t			from_len = sizeof(from);

			while (g_running) {
				ssize_t	ret = recvfrom(sock_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&from, &from_len);

				if (ret <= 0) break;

				struct ip		*ip_res = (struct ip *)recv_buf;
				int				hlen = ip_res->ip_hl << 2;
				struct icmphdr	*icmp_res = (struct icmphdr *)(recv_buf + hlen);

				if (icmp_res->type == ICMP_ECHOREPLY) {
					if (icmp_res->un.echo.id == getpid()) {
						if (icmp_res->un.echo.sequence == sequence - 1) {
							// 4.2 Set receive time and change stats values
							gettimeofday(&end, NULL);
							double diff = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;

							stats.received++;
							if (diff < stats.min) stats.min = diff;
							if (diff > stats.max) stats.max = diff;
							stats.sum += diff;
							stats.sum_sq += (diff * diff);

							// 4.3 Get ttl
							struct ip		*ip_hdr = (struct ip *)recv_buf;
							int				ttl = ip_hdr->ip_ttl;

							// 4.4 Send message
							printf(
									"%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
									total_size,
									inet_ntoa(from.sin_addr),
									sequence - 1,
									ttl,
									diff
							);

							break;
						}
					}
				} else {
					printf(
							"%ld bytes from %s: %s\n",
							ret - hlen,
							inet_ntoa(from.sin_addr),
							get_icmp_error_msg(icmp_res->type, icmp_res->code)
					);

					if (data.verbose)
						print_verbose(icmp_res);

					break;
				}
			}
			// 5. Waiting
			struct timeval	now;
			gettimeofday(&now, NULL);

			long	elapsed =	(now.tv_sec - start.tv_sec) * 1000000 +
								(now.tv_usec - start.tv_usec);
			
			if (elapsed < 1000000)
				usleep(1000000 - elapsed);
		}
		print_stats(stats);

		if (sock_fd != -1) close(sock_fd);
		if (res != NULL) freeaddrinfo(res);
		index++;
	}
}

