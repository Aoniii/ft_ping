#include "parser/parser.h"
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
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

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

	// 4. Init g_stats
	stats->addr = args;
	stats->min = 1e9;
	stats->max = 0;
	stats->sum = 0;
	stats->sum_sq = 0;
	gettimeofday(&stats->start, NULL);

	return (SUCCESS);
}

static void	setIPstr(struct addrinfo *res, char ip_str[][INET_ADDRSTRLEN]) {
	struct sockaddr_in	*ipv4 = (struct sockaddr_in *)res->ai_addr;
	inet_ntop(AF_INET, &(ipv4->sin_addr), *ip_str, INET_ADDRSTRLEN);
}

static void	print(t_stats stats) {
	struct timeval	end;
	gettimeofday(&end, NULL);

	int		loss = 0;
	if (stats.transmitted > 0)
		loss = (stats.transmitted - stats.received) * 100 / stats.transmitted;

	printf("--- %s ping statistics ---\n", stats.addr);
	printf(
			"%d packets transmitted, %d received, %d%% packet loss\n",
			stats.transmitted,
			stats.received,
			loss
	);

	if (stats.received > 0) {
		double	avg = stats.sum / stats.received;
		double	var = (stats.sum_sq / stats.received) - (avg * avg);
		double	mdev = sqrt(var < 0 ? 0 : var);

		printf(
				"round-trip min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
				stats.min,
				avg,
				stats.max,
				mdev
		);
	}
}

static const char	*get_icmp_error_msg(int type, int code) {
	for (size_t i = 0; i < sizeof(g_icmp_errors) / sizeof(g_icmp_errors[0]); i++) {
		if (g_icmp_errors[i].type == type && g_icmp_errors[i].code == code)
			return (g_icmp_errors[i].msg);
	}

	return ("Unknow ICMP Error");
}

void	ping(char **args, t_option *option) {
	(void)option;

	int				sock_fd = -1;
	struct addrinfo	hints;
	struct addrinfo	*res = NULL;
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

		memset(&stats, 0, sizeof(t_stats));
		if (init(&sock_fd, &hints, &res, args[index], &stats) != SUCCESS)
			break;

		signal(SIGINT, sig_handler);
		setIPstr(res, &ip_str);
		icmp = (struct icmphdr *)packet;

		printf("PING %s (%s): %d data bytes\n", args[index], ip_str, payload_size);
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
				perror("sendto");
				return;
			}

			// 4. Receive packet
			struct timeval		end;
			struct sockaddr_in	from;
			socklen_t			from_len = sizeof(from);
			ssize_t				ret = recvfrom(sock_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&from, &from_len);
			if (ret > 0) {
				// 4.1 Filtre
				struct ip		*ip_res = (struct ip *)recv_buf;
				int				hlen = ip_res->ip_hl << 2;
				struct icmphdr	*icmp_res = (struct icmphdr *)(recv_buf + hlen);

				if (icmp_res->type == ICMP_ECHOREPLY) {
					if (icmp_res->un.echo.id == getpid() && icmp_res->un.echo.sequence == sequence - 1) {
						// 4.2 Set receive time and change g_stats values
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
					}
				} else {
					printf(
							"%ld bytes from %s: %s\n",
							ret - hlen,
							inet_ntoa(from.sin_addr),
							get_icmp_error_msg(icmp_res->type, icmp_res->code)
					);
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
		print(stats);

		if (sock_fd != -1) close(sock_fd);
		if (res != NULL) freeaddrinfo(res);
		index++;
	}
}

