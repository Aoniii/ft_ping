#ifndef PING_H
# define PING_H

# include <sys/time.h>
# include <netinet/ip_icmp.h>
# include <stdbool.h>
# include <netdb.h>
# include <signal.h>

extern volatile sig_atomic_t g_running;

// Data structure
typedef struct	s_data {
	int			count;
	bool		verbose;
	int			linger;
	bool		flood;
	int			size;
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

void			ping(char **args, t_data data);
unsigned short	calculate_checksum(void *b, int len);
void			setIPstr(struct addrinfo *res, char ip_str[][INET_ADDRSTRLEN]);
const char		*get_icmp_error_msg(int type, int code);
void			print_stats(t_stats stats, t_data data);
void			print_verbose(struct icmphdr *icmp);
void			sig_handler(int sig);
t_error			check_data(t_data data);
void			callback_usage(void *data);
void			callback_version(void *data);

#endif
