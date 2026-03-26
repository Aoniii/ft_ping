#include "parser/parser.h"
#include "ping.h"
#include <stdio.h>
#include <stdbool.h>

volatile sig_atomic_t g_running = 1;
volatile sig_atomic_t g_waiting = 0;

int	main(int argc, char **argv) {
	t_parser_ctx	ctx;
	ctx.err = PARSER_SUCCESS;

	t_parser_info	info = {
		.program		= argv[0],
		.usage			= "[OPTION...] HOST ...",
		.description	= "Send ICMP ECHO_REQUEST packets to network hosts.\n",
		.footer			= "\nMandatory or optional arguments to long options are \
also mandatory or optional for any corresponding short options.\n\n\
Options marked with (root only) are available only to superuser.\n\n\
Report bugs to <https://github.com/Aoniii>."
	};

	t_data	data = {
		.verbose = false
	};

	t_option	option[] = {
		CATEGORY("Options valid for all request types:\n"),
		{
			.short_opt	= 'v',
			.long_opt	= "verbose",
			.flags		= OPT_SHORT | OPT_LONG | TYPE_BOOLEAN,
			.value		= &data.verbose,
			.help		= "verbose output"
		},
		{
			.short_opt	= '?',
			.long_opt	= "help",
			.flags		= OPT_SHORT | OPT_LONG | OPT_CALLBACK_EXIT | OPT_HIDDEN_HELP | TYPE_CALLBACK,
			.value		= (void *)&(t_callback_info){
				.fn = callback_help,
				.data = (void *)&(t_help_data){
					.info = info,
					.options = option
				}
			},
			.help		= ""
		},
		{0}
	};

	char	**args = parser(argc, argv, option, MODE_PERMISSIVE, &ctx);
	if (ctx.err != PARSER_SUCCESS) {
		error(info.program, &ctx);
		cleaner(args);
		return (ctx.err == CALLBACK_EXIT ? 0 : 1);
	}

	if (!args || !args[0]) {
		printf("ft_ping: missing host operand\n");
		printf("Try 'ping --help' for more information.\n");
		cleaner(args);
		return (1);
	}

	ping(args, data);

	cleaner(args);
	return (0);
}
