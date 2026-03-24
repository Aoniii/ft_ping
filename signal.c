#include "ping.h"

void	sig_handler(int sig) {
	(void)sig;
	g_running = false;
}

void	alarm_handler(int sig) {
	(void)sig;
	g_waiting = false;
}

