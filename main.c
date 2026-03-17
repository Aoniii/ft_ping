#include "parser/parser.h"

int	main(int argc, char **argv) {
	t_parser_ctx	ctx;
	ctx.err = PARSER_SUCCESS;

	t_parser_info	info = {
		.program		= argv[0],
		.usage			= "",
		.description	= ""
	};

	t_option	option[] = {
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
	};

	char	**args = parser(argc, argv, option, MODE_PERMISSIVE, &ctx);
	if (ctx.err != PARSER_SUCCESS) {
		error(info.program, &ctx);
		cleaner(args);
		return (ctx.err == CALLBACK_EXIT ? 0 : 1);
	}

	//exec

	cleaner(args);
	return (0);
}
