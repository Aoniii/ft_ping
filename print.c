#include "ping.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

void	print_stats(t_stats stats) {
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

void	print_verbose(struct icmphdr *icmp) {
	struct ip		*orig_ip = (struct ip *)((char *)icmp + sizeof(struct icmphdr));
	size_t			hlen = orig_ip->ip_hl << 2;
	unsigned char	*cp = (unsigned char *)orig_ip + hlen;

	printf("IP Hdr Dump:\n ");
	for (size_t i = 0; i < sizeof(struct ip); ++i)
		printf("%02x%s", *((unsigned char *)orig_ip + i), (i % 2) ? " " : "");
	printf("\n");

	printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src\tDst\tData\n");
	//Vr: version (ex: 4 = IPv4)
	printf(" %1x", orig_ip->ip_v);
	//HL: header length (generally 5)
	printf("  %1x", orig_ip->ip_hl);
	//TOS: Type of Service, package priority (generally 00)
	printf("  %02x", orig_ip->ip_tos);
	//Len: Total Length (header + data)
	printf(" %04x", (ntohs(orig_ip->ip_len) > 0x2000) ? orig_ip->ip_len : ntohs(orig_ip->ip_len));
	//ID: Identification (A unique identifier to help reconstruct the packet if it is fragmented)
	printf(" %04x", ntohs(orig_ip->ip_id));
	//Flg: Flags (Indicates whether the packet can be fragmented)
	printf("   %1x", (ntohs(orig_ip->ip_off) & 0xe000) >> 13);
	//off: Fragment Offset (If the packet is split, indicate where that fragment appears in the entire message)
	printf(" %04x", (ntohs(orig_ip->ip_off) & 0x1fff));
	//TTL: Time To Live (The maximum number of routers before the packet is discarded)
	printf("  %02x", orig_ip->ip_ttl);
	//Pro: Protocol (ex: 1 = ICMP, 6 = TCP)
	printf("  %02x", orig_ip->ip_p);
	//cks: Checksum (Checksum to verify that the IP header has not been corrupted)
	printf(" %04x", ntohs(orig_ip->ip_sum));
	//Src: Source
	printf(" %s ", inet_ntoa(orig_ip->ip_src));
	//Dst: Destination
	printf(" %s ", inet_ntoa(orig_ip->ip_dst));
	//Data: If HL > 5, the remaining bytes are options
	while(hlen-- > sizeof(struct ip))
		printf("%02x", *cp++);
	printf("\n");

	hlen = orig_ip->ip_hl << 2;
	cp = (unsigned char *)orig_ip + hlen;

	if (orig_ip->ip_p == IPPROTO_ICMP) {
		printf("ICMP: type %u, code %u, size %lu", cp[0], cp[1], ntohs(orig_ip->ip_len) - hlen);
		if (cp[0] == ICMP_ECHO || cp[0] == ICMP_ECHOREPLY)
			printf(", id 0x%02x%02x, seq 0x%02x%02x", cp[4], cp[5], cp[6], cp[7]);
		printf("\n");
	}
}

