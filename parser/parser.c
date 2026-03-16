#include "parser.h"
#include <stdbool.h>
#include <string.h>

static const t_option	*find_long_option(const t_option *options, char *key);
static const t_option	*find_short_option(const t_option *options, char c);

/**
 * @brief Command-line argument parsing engine.
 *
 * * Walks through argv and interprets each token according to a
 *   configuration table (t_option).
 * * Supports:
 *   - Long options:		--flag, --flag=value
 *   - Short options:		-f, -abc (bundled)  [implementation dependent]
 *   - Positional arguments mixed with options, depending on mode.
 *
 * @param argc		Argument count from main.
 * @param argv		Argument vector from main.
 * @param options	Pointer to a sentinel-terminated t_option array
 *					describing all supported flags.
 * @param mode		Parsing mode:
 *					- MODE_STRICT: stop option parsing at the first
 *					positional argument.
 *					- MODE_PERMISSIVE: allow options and positionals
 *					to be interleaved.
 * @param ctx		Output parameter for error reporting. Set to
 *					PARSER_SUCCESS on success, or one of the errors defined in t_parser_error.
 *
 * @return char**	A NULL-terminated, heap-allocated array of positional
 *					arguments on success, or NULL on critical error. The caller is responsible for freeing this array (see cleaner()).
 *
 * @note	The special token "--" stops option parsing and forces all remaining tokens to be treated as positional arguments, regardless of the current mode.
 */
char	**parser(int argc, char **argv, const t_option *options, int mode, t_parser_ctx *ctx) {
	int		index = 1;
	bool	force_arg = false;
	char	**args = NULL;

	while (index < argc) {
		char	*token = argv[index];

		if (ctx->err == CALLBACK_EXIT)
			return (args);

		ctx->token = token;
		ctx->opt = NULL;
		ctx->value = NULL;

		if (force_arg) {
			char	**tmp = append_arg(args, token, ctx);
			if (ctx->err == ERR_MALLOC_FAILED) return (tmp ? tmp : args);
			args = tmp;

			index++;
			continue;
		}

		/**		Force argument parsing		*/
		if (strlen(token) == 2 && strncmp(token, "--", 2) == 0) {
			force_arg = true;
			index++;
			continue;
		}

		/**		Long option parsing			*/
		if (strlen(token) > 2 && strncmp(token, "--", 2) == 0) {
			char *key = token + 2;
			char *equal_pos = strchr(token, '=');
			char *value = NULL;

			if (equal_pos) {
				*equal_pos = '\0';
				value = equal_pos + 1;
			}

			const t_option *opt = find_long_option(options, key);
			ctx->opt = opt;

			if (!opt) {
				if (equal_pos) *equal_pos = '=';
				ctx->err = ERR_UNKNOW_OPTION;
				return (args);
			}

			if (!equal_pos && (opt->flags & (TYPE_INT | TYPE_STRING | TYPE_UINT | TYPE_DOUBLE))) {
				if (index + 1 < argc) {
					value = argv[++index];
				} else {
					ctx->err = ERR_MISSING_VALUE;
					return (args);
				}
			}
			assign(opt, value, ctx);
    
			if (equal_pos) *equal_pos = '=';
			if (ctx->err != PARSER_SUCCESS) return (args);
    
			index++;
			continue;
		}

		/**		Short option parsing			*/
		if (strlen(token) > 1 && token[0] == '-') {
			char	*p = token + 1;

			while (*p) {
				const t_option	*opt = find_short_option(options, *p);
				ctx->opt = opt;

				if (!opt) {
					ctx->err = ERR_UNKNOW_OPTION;
					return (args);
				}

				char	*value = NULL;
				if (opt->flags & (TYPE_INT | TYPE_UINT | TYPE_STRING | TYPE_DOUBLE)) {
					if (*(p + 1)) {
						value = p + 1;
						assign(opt, value, ctx);
						break;
					} else if (index + 1 < argc) {
						index++;
						value = argv[index];
						assign(opt, value, ctx);
					} else {
						ctx->err = ERR_MISSING_VALUE;
						return (args);
					}
				} else {
					assign(opt, value, ctx);	
				}

				if (ctx->err != PARSER_SUCCESS)
					return (args);

				p++;
			}
			index++;
			continue;
		}

		char	**tmp = append_arg(args, token, ctx);
		if (ctx->err == ERR_MALLOC_FAILED) return (tmp ? tmp : args);
		args = tmp;

		index++;
		if (mode == MODE_STRICT)
			force_arg = true;
	}
	return (args);
}

static const t_option	*find_long_option(const t_option *options, char *key) {
	while (options) {
		if (options->flags == 0 && !options->short_opt && !options->long_opt)
			break;
		if (options->flags & OPT_CATEGORY) {
			options++;
			continue;
		}
		if (options->flags & OPT_LONG)
			if (options->long_opt)
				if (strcmp(options->long_opt, key) == 0)
					return (options);
		options++;
	}
	return (NULL);
}

static const t_option	*find_short_option(const t_option *options, char c) {
	while (options) {
		if (options->flags == 0 && !options->short_opt && !options->long_opt)
			break;
		if (options->flags & OPT_CATEGORY) {
			options++;
			continue;
		}
		if (options->flags & OPT_SHORT)
			if (options->short_opt == c)
            	return (options);
        options++;
    }
    return (NULL);
}
