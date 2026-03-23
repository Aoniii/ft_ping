#ifndef PING_H
# define PING_H

# include <bits/types/struct_timeval.h>
#include <netinet/ip_icmp.h>
# include <stdbool.h>

// Define t_option without include all parser header.
typedef struct s_option t_option;

// Stats structure
typedef struct		s_stats {
	char			*addr;
	int				transmitted;
	int				received;
	double			min;
	double			max;
	double			sum;
	double			sum_sq;
	struct timeval	start;
}					t_stats;

// Enum error
typedef enum	e_error {
	SUCCESS = 0,
	ERROR,
}				t_error;

// ICMP error
struct			s_icmp_err {
    int			type;
    int			code;
    const char	*msg;
};

static const struct s_icmp_err g_icmp_errors[] = {
    {ICMP_DEST_UNREACH, ICMP_NET_UNREACH, "Destination Net Unreachable"},
    {ICMP_DEST_UNREACH, ICMP_HOST_UNREACH, "Destination Host Unreachable"},
    {ICMP_DEST_UNREACH, ICMP_PROT_UNREACH, "Destination Protocol Unreachable"},
    {ICMP_DEST_UNREACH, ICMP_PORT_UNREACH, "Destination Port Unreachable"},
    {ICMP_DEST_UNREACH, ICMP_FRAG_NEEDED, "Fragmentation needed and DF set"},
    {ICMP_DEST_UNREACH, ICMP_SR_FAILED, "Source Route Failed"},
    {ICMP_TIME_EXCEEDED, ICMP_EXC_TTL, "Time to live exceeded"},
    {ICMP_TIME_EXCEEDED, ICMP_EXC_FRAGTIME, "Frag reassembly time exceeded"},
    {ICMP_PARAMETERPROB, 0, "Parameter problem"},
    {ICMP_SOURCE_QUENCH, 0, "Source Quench (Congestion control)"}
};

// Global variable for running.
extern bool		g_running;

void	ping(char **args, t_option *option);
void	sig_handler(int sig);

#endif
