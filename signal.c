#include "ping.h"

void	sig_handler(int sig) {
	(void)sig;
	g_running = 0;
}

void	alarm_handler(int sig) {
	(void)sig;
	g_waiting = 0;
}

