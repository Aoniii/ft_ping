#include "parser.h"
#include <stdio.h>

void	error(const char *program, t_parser_ctx *ctx) {
	switch (ctx->err) {
		case ERR_UNKNOW_OPTION:
			printf("%s: unrecognized option ('%s')\n", program, ctx->token);
			break;
		case ERR_MISSING_VALUE:
			printf("%s: option ('%s') requires an argument\n", program, ctx->token);
			break;
		case ERR_INVALID_FORMAT:
			printf("%s: invalid value ('%s')\n", program, ctx->value);
			break;
		case ERR_OVERFLOW:
			printf("%s: value overflow of option ('%s')\n", program, ctx->token);
			break;
		case ERR_MALLOC_FAILED:
			printf("%s: allocation failed\n", program);
			break;
		default:
			break;
	}
}

