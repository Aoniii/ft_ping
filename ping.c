#include "ping.h"
#include <netdb.h>
#include <signal.h>
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
#include <poll.h>

/**
 * @brief Initialize the socket and the address information
 * 
 * Initializes a ping session for a given host. Creates a raw ICMP socket, resolves the target hostname/IP using getaddrinfo,
 * and zeroes out the statistics structure (min/max RTT, counters, start time). Returns ERROR if the socket creation or DNS
 * resolution fails, SUCCESS otherwise.
 *
 * @param sock_fd The socket file descriptor
 * @param hints The hints for the address information
 * @param res The address information
 * @param args The target hostname/IP
 * @param stats The statistics
 */
static t_error	init(int *sock_fd, struct addrinfo *hints, struct addrinfo **res, char *args, t_stats *stats) {
	if ((*sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		perror("socket");
		return (ERROR);
	}

	memset(hints, 0, sizeof(*hints));
	(*hints).ai_family = AF_INET;
	(*hints).ai_socktype = SOCK_RAW;

	if ((getaddrinfo(args, NULL, hints, res)) != 0) {
		if (!g_running) printf("\n");
		else fprintf(stderr, "ft_ping: unknown host\n");
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

/**
 * @brief Get the display address
 * 
 * Returns a display string for the source address of a received packet. When -n is not set, performs a reverse DNS lookup using getnameinfo.
 * The commented-out line would format the output as hostname (ip) to match inetutils-2.0 display, but currently only the IP is shown to match
 * the subject requirement of not doing DNS resolution in the packet return. When -n is set or the reverse lookup fails, returns the raw IP address.
 *
 * @param from The source address
 * @param data The data (numeric flag)
 * @return char* The display address
 */
static char	*get_display_addr(struct sockaddr_in *from, t_data data) {
	static char	display[NI_MAXHOST + INET_ADDRSTRLEN + 4];
	char		host[NI_MAXHOST];
 
	if (!data.numeric && getnameinfo((struct sockaddr *)from, sizeof(*from), host, sizeof(host), NULL, 0, NI_NAMEREQD) == 0) {
		//snprintf(display, sizeof(display), "%s (%s)", host, inet_ntoa(from->sin_addr));
		snprintf(display, sizeof(display), "%s", inet_ntoa(from->sin_addr));
		return (display);
	}
	return (inet_ntoa(from->sin_addr));
}

/**
 * @brief Handle the response
 * 
 * Parses and processes a received ICMP packet. Validates the IP header and ICMP header lengths (reporting errors in verbose mode).
 * For ICMP_ECHOREPLY matching our PID, computes the round-trip time from the embedded timestamp, updates statistics (min/max/sum/sum_sq),
 * and prints the response line (or backspace in flood mode). For other ICMP types (except ICMP_ECHO which is silently ignored),
 * prints the error message and, in verbose mode, dumps the original IP/ICMP headers of the offending packet.
 * Uses get_display_addr for source address formatting, supporting the -n flag.
 *
 * @param ret The return value (length of the received packet)
 * @param recv_buf The receive buffer
 * @param from The source address
 * @param stats The statistics
 * @param data The data
 */
static void	handle_response(int ret, char *recv_buf, struct sockaddr_in *from, t_stats *stats, t_data data) {
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
		if (icmp_res->un.echo.id == (uint16_t)(getpid() & 0xFFFF)) {
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

			if (data.flood) {
				putchar('\b');
			} else {
				printf(
					"%d bytes from %s: icmp_seq=%d ttl=%d",
					ret - hlen,
					get_display_addr(from, data),
					ntohs(icmp_res->un.echo.sequence),
					ip_res->ip_ttl
				);
				if (diff != -1) printf(" time=%.3f ms", diff);
				printf("\n");
			}
		}
	} else {
		if (icmp_res->type == ICMP_ECHO) return;

		printf(
				"%d bytes from %s: %s\n",
				ret - hlen,
				get_display_addr(from, data),
				get_icmp_error_msg(icmp_res->type, icmp_res->code)
		);
		
		if (data.verbose) {
			int icmp_payload_len = ret - hlen - (int)sizeof(struct icmphdr);

			if (icmp_payload_len >= (int)sizeof(struct ip) + (int)sizeof(struct icmphdr)) print_verbose(icmp_res);
			else fprintf(stderr, "ft_ping: verbose: truncated ICMP payload, cannot print headers\n");
		}
	}
}

/**
 * @brief Send a ping packet
 * 
 * Builds and sends an ICMP Echo Request packet. Constructs the packet with the ICMP header (type, PID-based identifier, sequence number in network byte order),
 * embeds the current timestamp in the payload for RTT calculation, computes the ICMP checksum, and sends it to the destination via sendto.
 * Returns ERROR if sending fails, SUCCESS otherwise.
 *
 * @param sock_fd The socket file descriptor
 * @param res The address information
 * @param seq The sequence number
 * @param total_size The total size of the packet
 */
static t_error	send_ping(int sock_fd, struct addrinfo *res, int seq, int total_size) {
	char			packet[IP_MAXPACKET];
	struct icmphdr	*icmp = (struct icmphdr *)packet;

	memset(packet, 0, total_size);
	icmp->type = ICMP_ECHO;
	icmp->un.echo.id = (uint16_t)(getpid() & 0xFFFF);
	icmp->un.echo.sequence = htons(seq);

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

/**
 * @brief Receive one packet
 * 
 * Receives a single ICMP packet from the socket using recvfrom. Silently ignores interrupted or non-blocking (EINTR/EAGAIN) errors.
 * On fatal receive errors, prints the error and signals the program to stop. On successful reception, delegates packet processing to handle_response.
 *
 * @param sock_fd The socket file descriptor
 * @param recv_buf The receive buffer
 * @param from The source address
 * @param stats The statistics
 * @param data The data
 */
static void	recv_one(int sock_fd, char *recv_buf, struct sockaddr_in *from, t_stats *stats, t_data data) {
	socklen_t	from_len = sizeof(*from);
	ssize_t		ret = recvfrom(sock_fd, recv_buf, IP_MAXPACKET, 0, (struct sockaddr *)from, &from_len);

	if (ret < 0) {
		if (errno == EINTR || errno == EAGAIN)
			return;
		perror("recvfrom");
		g_running = 0;
		return;
	}
	if (ret > 0)
		handle_response((int)ret, recv_buf, from, stats, data);
}

/**
 * @brief Ping loop for flood mode
 * 
 * Implements the flood ping mode (-f). Sends packets as fast as possible (with a 10ms usleep between sends), printing a dot for each sent packet.
 * Uses poll with a short timeout (10ms) to check for responses between sends, draining all available replies in a non-blocking loop.
 * After the send loop ends (by count limit or SIGINT), waits up to 300ms to collect any remaining in-flight responses.
 *
 * @param sock_fd The socket file descriptor
 * @param res The address information
 * @param stats The statistics
 * @param data The data
 * @param total_size The total size of the packet
 */
static void	ping_loop_flood(int sock_fd, struct addrinfo *res, t_stats *stats, t_data data, int total_size) {
	char				recv_buf[IP_MAXPACKET];
	struct sockaddr_in	from;
	int					sequence = 0;
	struct pollfd		fds[1];
	int					count = 0;

	fds[0].fd = sock_fd;
	fds[0].events = POLLIN;

	while (g_running) {
		if (send_ping(sock_fd, res, sequence++, total_size) == ERROR)
			break;
		stats->transmitted++;
		putchar('.');

		int poll_ret = poll(fds, 1, 10);

		if (poll_ret < 0) {
			if (errno == EINTR)
				continue;
			perror("poll");
			g_running = 0;
			break;
		}

		while (poll_ret > 0) {
			recv_one(sock_fd, recv_buf, &from, stats, data);
			poll_ret = poll(fds, 1, 0);
		}

		if (poll_ret < 0 && errno != EINTR) {
			perror("poll");
			g_running = 0;
			break;
		}

		count++;
		if (data.count > 0 && count >= data.count) break;
		usleep(10000);
	}

	while (g_running) {
		int	poll_ret = poll(fds, 1, 300);
		if (poll_ret <= 0)
			break;
		recv_one(sock_fd, recv_buf, &from, stats, data);
	}

	printf("\b");
}

/**
 * @brief Ping loop for normal mode
 * 
 * Main ping loop for normal (non-flood) mode. Sends one packet per second and waits for replies using poll with a time-based timeout.
 * For the last packet (when -c count is set), uses the linger timeout (-W) instead of the default 1-second interval to allow extra time for a late response.
 * Tracks elapsed time precisely to maintain consistent intervals regardless of reply timing. Exits on SIGINT, send failure, or when the packet count is reached.
 *
 * @param sock_fd The socket file descriptor
 * @param res The address information
 * @param stats The statistics
 * @param data The data
 * @param total_size The total size of the packet
 */
static void	ping_loop(int sock_fd, struct addrinfo *res, t_stats *stats, t_data data, int total_size) {
	char				recv_buf[IP_MAXPACKET];
	struct sockaddr_in	from;
	int					sequence = 0;
	struct pollfd		fds[1];
	int					count = 0;

	fds[0].fd = sock_fd;
	fds[0].events = POLLIN;

	while (g_running) {
		struct timeval	send_time;
		gettimeofday(&send_time, NULL);

		if (send_ping(sock_fd, res, sequence++, total_size) == ERROR)
			break;
		stats->transmitted++;

		bool	last_packet = (data.count > 0 && count + 1 >= data.count);
		int		wait_ms = last_packet ? data.linger * 1000 : 1000;

		while (g_running) {
			struct timeval	now;
			gettimeofday(&now, NULL);

			int	elapsed =	(now.tv_sec - send_time.tv_sec) * 1000
							+ (now.tv_usec - send_time.tv_usec) / 1000;
			int	timeout = wait_ms - elapsed;
			if (timeout <= 0) break;

			int poll_ret = poll(fds, 1, timeout);
			if (poll_ret < 0) {
				if (errno == EINTR) break;
				perror("poll");
				g_running = 0;
				break;
			}
			if (poll_ret == 0)
				break;

			recv_one(sock_fd, recv_buf, &from, stats, data);

			if (last_packet && stats->received >= stats->transmitted) break;
		}
		count++;
		if (data.count > 0 && count >= data.count) break;
	}
}

/**
 * @brief Ping the target
 * 
 * Entry point for the ping process. Computes the total packet size (ICMP header + data), registers the SIGINT handler, then iterates over each target host.
 * For each host, initializes the socket and resolves the address via init, prints the PING header line (with extra ID info in verbose mode),
 * runs the appropriate loop (flood or normal), prints final statistics, and cleans up resources. Supports multiple hosts as arguments.
 *
 * @param args The target hostname/IPs
 * @param data The data
 */
void	ping(char **args, t_data data) {
	int	index = 0;
	int	total_size = sizeof(struct icmphdr) + data.size;

	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		perror("signal");
		return;
	}

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

		if (data.flood) ping_loop_flood(sock_fd, res, &stats, data, total_size);
		else ping_loop(sock_fd, res, &stats, data, total_size);

		print_stats(stats, data);
		freeaddrinfo(res);
		close(sock_fd);
		index++;
	}
}
