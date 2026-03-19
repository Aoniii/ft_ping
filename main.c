#include "parser/parser.h"
#include "ping.h"
#include <stdbool.h>

t_stats	g_stats = {0};
char	**g_args = 0;

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

	bool	verbose = false;

	t_option	option[] = {
		CATEGORY("Options valid for all request types:\n"),
		{
			.short_opt	= 'v',
			.long_opt	= "verbose",
			.flags		= OPT_SHORT | OPT_LONG | TYPE_BOOLEAN,
			.value		= &verbose,
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

	g_args = args;
	ping(args, option);

	cleaner(args);
	return (0);
}
