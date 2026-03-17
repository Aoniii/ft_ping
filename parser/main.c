#include "parser.h"
#include <stdbool.h>
#include <stddef.h>

int	main(int argc, char **argv) {
	/**		Error			*/
	t_parser_ctx	ctx;
	ctx.err = PARSER_SUCCESS;

	/**		Information		*/
	t_parser_info	info = {
		.program		= argv[0],
		.usage			= "[Example...] Example ...",
		.description	= "Description example",
		.footer			= "Footer example"
	};

	/**		Options			*/
	bool			b = false;
	int				i = 0;
	unsigned int	u = 0;
	double			d = 0;
	char			*s = 0;
	int				c = 0;

	t_option option[] = {
		CATEGORY("Category example:"),
		{
			.short_opt	= 'b',
			.long_opt	= "bool",
			.flags		= OPT_SHORT | OPT_LONG | TYPE_BOOLEAN,
			.value		= &b,
			.help		= "Example of a boolean variable"
		},
		{
			.short_opt	= 'i',
			.long_opt	= "int",
			.flags		= OPT_SHORT | OPT_LONG | TYPE_INT,
			.value		= &i,
			.help		= "Example of an integer variable"
		},
		{
			.short_opt	= 'u',
			.long_opt	= "uint",
			.flags		= OPT_SHORT | OPT_LONG | TYPE_UINT,
			.value		= &u,
			.help		= "Example of an unsigned integer variable"
		},
		{
			.short_opt	= 'd',
			.long_opt	= "double",
			.flags		= OPT_SHORT | OPT_LONG | TYPE_DOUBLE,
			.value		= &d,
			.help		= "Example of a double variable"
		},
		{
			.short_opt	= 's',
			.long_opt	= "string",
			.flags		= OPT_SHORT | OPT_LONG | TYPE_STRING,
			.value		= &s,
			.help		= "Example of a string variable"
		},
		{
			.short_opt	= 'c',
			.long_opt	= "count",
			.flags		= OPT_SHORT | OPT_LONG | TYPE_COUNT,
			.value		= &c,
			.help		= "Example of a counter variable"
		},
		{
			.short_opt	= 'h',
			.long_opt	= "help",
			.flags		= OPT_SHORT | OPT_LONG | OPT_CALLBACK_EXIT | OPT_HIDDEN_HELP | TYPE_CALLBACK,
			.value		= (void *)&(t_callback_info){
				.fn = callback_help,
				.data = (void *)&(t_help_data){
					.info = info,
					.options = option
				}
			},
			.help		= "Example of a callback"
		},
		{0}
	};

	/**		Parsing			*/
	char	**args = parser(argc, argv, option, MODE_PERMISSIVE, &ctx);
	if (ctx.err != PARSER_SUCCESS) {
		error(info.program, &ctx);
		cleaner(args);
		return (ctx.err == CALLBACK_EXIT ? 0 : 1);
	}

	/**		Execution		*/
	debug(args, option);

	cleaner(args);
    return (0);
}
