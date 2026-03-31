#include "ping.h"
#include <stdio.h>
#include <limits.h>

t_error check_data(t_data data) {
	if (data.linger <= 0) {
		fprintf(stderr, "ft_ping: option value too small: '%d'\n", data.linger);
		return (ERROR);
	}

	if (data.size >= 65400) {
		fprintf(stderr, "ft_ping: option value too big: '%d'\n", data.size);
		return (ERROR);
	}
	return (SUCCESS);
}