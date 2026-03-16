#include "parser.h"
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

typedef void		(*t_assign_fn)(const t_option *options, char *value, t_parser_ctx *ctx);

typedef struct		s_assign_entry {
	int				type_flag;
	t_assign_fn		fn;
}					t_assign_entry;

static void	assign_boolean(const t_option *options, char *value, t_parser_ctx *ctx);
static void	assign_int(const t_option *options, char *value, t_parser_ctx *ctx);
static void	assign_uint(const t_option *options, char *value, t_parser_ctx *ctx);
static void	assign_double(const t_option *options, char *value, t_parser_ctx *ctx);
static void	assign_string(const t_option *options, char *value, t_parser_ctx *ctx);
static void	assign_count(const t_option *options, char *value, t_parser_ctx *ctx);
static void	assign_callback(const t_option *options, char *value, t_parser_ctx *ctx);

/**
 * @brief Assigns an option's value to its typed target.
 * * Based on the type flags (TYPE_*), this function converts the
 *   string value and writes the result into options->value, or
 *   triggers a specific behavior (counter, callback, boolean).
 * * Supported types:
 *   - TYPE_BOOLEAN		: sets the target value to true, ignores value.
 *   - TYPE_INT			: parses a signed integer.
 *   - TYPE_UINT		: parses an unsigned integer.
 *   - TYPE_DOUBLE		: parses a floating point number.
 *   - TYPE_STRING		: stores the raw pointer to value.
 *   - TYPE_COUNT		: increments an integer counter.
 *   - TYPE_CALLBACK	: calls the function stored in options->value.
 * * On parsing error or missing value, err is updated with the
 *   appropriate error code (ERR_MISSING_VALUE, ERR_INVALID_FORMAT, ...).
 *
 * @param options	Pointer to the option description (t_option).
 * @param value		Raw string associated with the option (or NULL if none).
 * @param ctx		Pointer to the parser context.
 */
void	assign(const t_option *options, char *value, t_parser_ctx *ctx) {
	if (!options || !options->value)
		return;

	static const t_assign_entry	assign_table[] = {	
		{ TYPE_BOOLEAN,		assign_boolean },
		{ TYPE_INT,			assign_int },
		{ TYPE_UINT,		assign_uint },
		{ TYPE_DOUBLE,		assign_double },
		{ TYPE_STRING,		assign_string },
		{ TYPE_COUNT,		assign_count },
		{ TYPE_CALLBACK,	assign_callback },
		{ 0, NULL }
	};

	int	index = 0;
	while (assign_table[index].type_flag) {
		if (options->flags & assign_table[index].type_flag) {
			assign_table[index].fn(options, value, ctx);
			return;
		}
		index++;
	}
}

static void	assign_boolean(const t_option *options, char *value, t_parser_ctx *ctx) {
	(void)value;
	(void)ctx;
	*((bool *)options->value) = true;
}

static void	assign_int(const t_option *options, char *value, t_parser_ctx *ctx) {
	if (!value) {
		ctx->err = ERR_MISSING_VALUE;
		ctx->value = value;
		return;
	}

	errno = 0;
	char	*endptr = NULL;
	long	result = strtol(value, &endptr, 10);

	if (*endptr != '\0' || endptr == value) {
        ctx->err = ERR_INVALID_FORMAT;
		ctx->value = value;
        return;
    }

	/**
	 * Smart saturation
	 * We handle the case where strtol itself overflow (ERANGE)
	 * and the case where the length is valid but too large or too small for an int.
	 */
    if (errno == ERANGE) {
        if (result > 0)
            *((int *)options->value) = INT_MAX;
        else
            *((int *)options->value) = INT_MIN;
    } else if (result > INT_MAX) {
        *((int *)options->value) = INT_MAX;
    } else if (result < INT_MIN) {
        *((int *)options->value) = INT_MIN;
    } else {
        *((int *)options->value) = (int)result;
    }
}

static void	assign_uint(const t_option *options, char *value, t_parser_ctx *ctx) {
	if (!value) {
		ctx->err = ERR_MISSING_VALUE;
		ctx->value = value;
		return;
	}
	
	char	*temp = value;
    while (*temp == ' ' || (*temp >= '\t' && *temp <= '\r')) 
        temp++;
    if (*temp == '-') {
        ctx->err = ERR_INVALID_FORMAT;
		ctx->value = value;
        return;
    }

	errno = 0;
    char          *endptr = NULL;
    unsigned long result = strtoul(value, &endptr, 10);

    if (*endptr != '\0' || endptr == value) {
        ctx->err = ERR_INVALID_FORMAT;
		ctx->value = value;
        return;
    }

	/**
	 * Smart saturation
	 * If strtoul detects an overflow (ERANGE) or if result
	 * exceeds the range of an unsigned int.
	 */
    if (errno == ERANGE || result > UINT_MAX) {
        *((unsigned int *)options->value) = UINT_MAX;
    } else {
        *((unsigned int *)options->value) = (unsigned int)result;
    }
}

static void	assign_double(const t_option *options, char *value, t_parser_ctx *ctx) {
	if (!value) {
		ctx->err = ERR_MISSING_VALUE;
		ctx->value = value;
		return;
	}

	errno = 0;
	char	*endptr = NULL;
	double	result = strtod(value, &endptr);

	if (*endptr != '\0' || endptr == value) {
		ctx->err = ERR_INVALID_FORMAT;
		ctx->value = value;
		return;
	}

	if (errno == ERANGE) {
		ctx->err = ERR_INVALID_FORMAT;
		ctx->value = value;
		return;
	}

	*((double *)options->value) = result;
}

static void	assign_string(const t_option *options, char *value, t_parser_ctx *ctx) {
	if (!value) {
		ctx->err = ERR_MISSING_VALUE;
		ctx->value = value;
		return;
	}

	*((char **)options->value) = value;
}

static void	assign_count(const t_option *options, char *value, t_parser_ctx *ctx) {
	(void)value;
	(void)ctx;

	if (*((int *)options->value) == INT_MAX) {
		ctx->err = ERR_OVERFLOW;
		return;
	}

	(*((int *)options->value))++;
}

static void	assign_callback(const t_option *options, char *value, t_parser_ctx *ctx) {
	(void)value;
	(void)ctx;
	t_callback_info *callback = (t_callback_info *)options->value;

	if (callback && callback->fn) {
		callback->fn(callback->data);
		
		if (options->flags & OPT_CALLBACK_EXIT)
			ctx->err = CALLBACK_EXIT;
	}
}
