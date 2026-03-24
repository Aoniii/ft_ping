#ifndef PING_H
# define PING_H

# include <sys/time.h>
# include <netinet/ip_icmp.h>
# include <stdbool.h>
# include <netdb.h>

// Data structure
typedef struct	s_data {
	bool		verbose;
}				t_data;

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

// Global variable for running and waiting.
extern bool		g_running;
extern bool		g_waiting;

void			ping(char **args, t_data data);

unsigned short	calculate_checksum(void *b, int len);
void			setIPstr(struct addrinfo *res, char ip_str[][INET_ADDRSTRLEN]);
const char		*get_icmp_error_msg(int type, int code);

void			print_stats(t_stats stats);
void			print_verbose(struct icmphdr *icmp);

void			sig_handler(int sig);
void			alarm_handler(int sig);

#endif
