#ifndef PING_H
# define PING_H

# include <bits/types/struct_timeval.h>
# include <stdbool.h>

typedef struct s_option t_option;

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

typedef enum	e_error {
	SUCCESS = 0,
	ERROR,
}				t_error;

extern bool		g_running;

void	ping(char **args, t_option *option);
void	sig_handler(int sig);

#endif
