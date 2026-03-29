#include "parser.h"
#include <stdio.h>

static void help(const t_parser_info info, const t_option *options);

void	callback_help(void *data) {
	t_help_data *callback = (t_help_data *)data;
	help(callback->info, callback->options);
}

static void help(const t_parser_info info, const t_option *options) {
	char		buf[128];

	printf("Usage: %s %s\n", info.program, info.usage);
	printf("%s\n", info.description);

	for (const t_option *opt = options; ; opt++) {
		if (opt->flags == 0 && !opt->short_opt && !opt->long_opt)
			break;
		if (opt->flags & OPT_CATEGORY) {
			printf("%s", opt->help);
			continue;
		}
		if (opt->flags & OPT_HIDDEN_HELP)
			continue;

		if (opt->short_opt && opt->long_opt)
			snprintf(buf, sizeof(buf), "  -%c, --%-20s", opt->short_opt, opt->long_opt);
		else if (opt->short_opt)
			snprintf(buf, sizeof(buf), "  -%c", opt->short_opt);
		else if (opt->long_opt)
			snprintf(buf, sizeof(buf), "     --%-20s", opt->long_opt);
		else
			continue;
		printf("%s %s\n", buf, opt->help);
	}

	printf("%s\n", info.footer);
}
