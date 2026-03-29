#include "ping.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

static t_error	init(int *sock_fd, struct addrinfo *hints, struct addrinfo **res, char *args, t_stats *stats) {
	if ((*sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		perror("socket");
		return (ERROR);
	}

	struct timeval	tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(*sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("setsockopt");
		close(*sock_fd);
		return (ERROR);
	}

	memset(hints, 0, sizeof(*hints));
	(*hints).ai_family = AF_INET;
	(*hints).ai_socktype = SOCK_RAW;

	if (getaddrinfo(args, NULL, hints, res) != 0) {
		fprintf(stderr, "ft_ping: unknown host\n");
		close(*sock_fd);
		return (ERROR);
	}

	stats->addr = args;
	stats->min = 1e9;
	stats->max = 0;
	stats->sum = 0;
	stats->sum_sq = 0;
	stats->transmitted = 0;
	stats->received = 0;
	gettimeofday(&stats->start, NULL);

	return (SUCCESS);
}

static void	handle_response(int ret, char *recv_buf, struct sockaddr_in *from, t_stats *stats, t_data data, int expected_seq) {
	if (ret < (int)sizeof(struct ip)) {
		if (data.verbose)
			fprintf(stderr, "ft_ping: packet too short (%d bytes) for IP header\n", ret);
		return;
	}

	struct ip		*ip_res = (struct ip *)recv_buf;
	int				hlen = ip_res->ip_hl << 2;

	if (hlen < (int)sizeof(struct ip) || hlen > ret) {
		if (data.verbose)
			fprintf(stderr, "ft_ping: invalid IP header length (%d bytes)\n", hlen);
		return;
	}

	if (ret < hlen + (int)sizeof(struct icmphdr)) {
		if (data.verbose)
			fprintf(stderr, "ft_ping: packet too short for ICMP header\n");
		return;
	}

	struct icmphdr	*icmp_res = (struct icmphdr *)(recv_buf + hlen);
	struct timeval	*start, end;

	if (icmp_res->type == ICMP_ECHOREPLY) {
		if (icmp_res->un.echo.id == (uint16_t)(getpid() & 0xFFFF) && icmp_res->un.echo.sequence == (uint16_t)expected_seq) {
			int	payload_len = ret - hlen - (int)sizeof(struct icmphdr);
			double diff = -1;
			if (payload_len >= (int)sizeof(struct timeval)) {
				start = (struct timeval *)(recv_buf + hlen + sizeof(struct icmphdr));

				gettimeofday(&end, NULL);
				diff = (end.tv_sec - start->tv_sec) * 1000.0 + (end.tv_usec - start->tv_usec) / 1000.0;

				if (diff < stats->min) stats->min = diff;
				if (diff > stats->max) stats->max = diff;
				stats->sum += diff;
				stats->sum_sq += (diff * diff);
			}
			stats->received++;

			printf(
				"%d bytes from %s: icmp_seq=%d ttl=%d",
				ret - hlen,
				inet_ntoa(from->sin_addr),
				icmp_res->un.echo.sequence,
				ip_res->ip_ttl
			);
			if (diff > 0) printf(" time=%.3f ms", diff);
			printf("\n");
		}
	} else {
		printf("%d bytes from %s: %s\n", ret - hlen, inet_ntoa(from->sin_addr), 
			   get_icmp_error_msg(icmp_res->type, icmp_res->code));
		
		if (data.verbose) {
			int icmp_payload_len = ret - hlen - (int)sizeof(struct icmphdr);

			if (icmp_payload_len >= (int)sizeof(struct ip) + (int)sizeof(struct icmphdr)) print_verbose(icmp_res);
			else fprintf(stderr, "ft_ping: verbose: truncated ICMP payload, cannot print headers\n");
		}
	}
}

static t_error	send_ping(int sock_fd, struct addrinfo *res, int seq, int total_size) {
	char			packet[IP_MAXPACKET];
	struct icmphdr	*icmp = (struct icmphdr *)packet;

	memset(packet, 0, total_size);
	icmp->type = ICMP_ECHO;
	icmp->un.echo.id = (uint16_t)(getpid() & 0xFFFF);
	icmp->un.echo.sequence = seq;

	struct timeval	now;
	gettimeofday(&now, NULL);
	struct timeval	*ts = (struct timeval *)(packet + sizeof(struct icmphdr));
	*ts = now;

	icmp->checksum = calculate_checksum(packet, total_size);

	if (sendto(sock_fd, packet, total_size, 0, res->ai_addr, res->ai_addrlen) <= 0) {
		perror("sendto");
		return (ERROR);
	}
	return (SUCCESS);
}

static void ping_loop(int sock_fd, struct addrinfo *res, t_stats *stats, t_data data, int total_size) {
	char				recv_buf[IP_MAXPACKET];
	struct sockaddr_in	from;
	socklen_t			from_len;
	int					sequence = 0;

	while (g_running) {
		if (!g_waiting) {
			if (send_ping(sock_fd, res, sequence++, total_size) == ERROR)
				break;
			stats->transmitted++;
			g_waiting = 1;
			alarm(1);
		}

		from_len = sizeof(from);
		ssize_t ret = recvfrom(sock_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&from, &from_len);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			perror("recvfrom");
			break;
		}
	
		if (ret > 0) {
			handle_response(ret, recv_buf, &from, stats, data, sequence - 1);
		}
	}
}

void	ping(char **args, t_data data) {
	int	index = 0;
	int	total_size = sizeof(struct icmphdr) + data.size;

	signal(SIGALRM, alarm_handler);
	signal(SIGINT, sig_handler);

	while (args[index] && g_running) {
		int				sock_fd;
		struct addrinfo	hints, *res;
		t_stats			stats;
		char			ip_str[INET_ADDRSTRLEN];

		if (init(&sock_fd, &hints, &res, args[index], &stats) != SUCCESS) {
			index++;
			continue;
		}

		setIPstr(res, &ip_str);
		printf("PING %s (%s): %d data bytes", args[index], ip_str, data.size);
		if (data.verbose)
			printf(", id 0x%04x = %u", getpid() & 0xFFFF, getpid() & 0xFFFF);
		printf("\n");

		ping_loop(sock_fd, res, &stats, data, total_size);

		print_stats(stats, data);
		freeaddrinfo(res);
		close(sock_fd);
		index++;
	}
}
