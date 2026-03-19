#include "parser/parser.h"
#include "ping.h"
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>

void	sig_handler(int sig) {
	(void)sig;
	
	struct timeval	end;
	gettimeofday(&end, NULL);

	int		loss = 0;
	if (g_stats.transmitted > 0)
		loss = (g_stats.transmitted - g_stats.received) * 100 / g_stats.transmitted;

	printf("--- %s ping statistics ---\n", g_stats.addr);
	printf(
			"%d packets transmitted, %d received, %d%% packet loss\n",
			g_stats.transmitted,
			g_stats.received,
			loss
	);

	if (g_stats.received > 0) {
		double	avg = g_stats.sum / g_stats.received;
		double	mdev = sqrt((g_stats.sum_sq / g_stats.received) - (avg * avg));

		printf(
				"round-trip min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
				g_stats.min,
				avg,
				g_stats.max,
				mdev
		);
	}

	cleaner(g_args);
	exit(0);
}

