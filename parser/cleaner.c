#include <stdlib.h>

void	cleaner(char **args) {
	int	index = 0;

	if (!args)
		return;

	while (args[index]) {
		if (args[index]) {
			free(args[index]);
			args[index] = NULL;
		}
		index++;
	}
	free(args);
	args = NULL;
}
