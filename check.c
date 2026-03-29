#include "ping.h"
#include <stdio.h>

t_error check_data(t_data data) {
	if (data.size >= 65400) {
		fprintf(stderr, "ft_ping: option value too big: '%d'\n", data.size);
		return (ERROR);
	}
	return (SUCCESS);
}