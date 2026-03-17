#ifndef PARSER_H
# define PARSER_H

/**		Parser modes			*/
# define MODE_STRICT			0x0001
# define MODE_PERMISSIVE		0x0002

/**		Data types				*/
# define OPT_SHORT				0x0010
# define OPT_LONG				0x0020
# define OPT_CALLBACK_EXIT		0x0040
# define OPT_HIDDEN_HELP		0x0080

/**		Flag types				*/
# define TYPE_BOOLEAN			0x0100
# define TYPE_INT				0x0200
# define TYPE_UINT				0x0400
# define TYPE_DOUBLE			0x0800
# define TYPE_STRING			0x1000
# define TYPE_COUNT				0x2000
# define TYPE_CALLBACK			0x4000

/**		Categories help			*/
# define OPT_CATEGORY			(1 << 15)
# define CATEGORY(title)		{ .flags = OPT_CATEGORY, .help = title }

/**		Parser struct			*/
typedef struct	s_parser_info {
	const char	*program;
	const char	*usage;
	const char	*description;
	const char	*footer;
}				t_parser_info;

typedef struct	s_option {
	char		short_opt;
	const char	*long_opt;
	int			flags;
	void		*value;
	const char	*help;
}				t_option;

/**		Callback fonctions		*/
typedef void	(*t_callback)(void *data);

typedef struct	s_callback_info {
	t_callback	fn;
	void		*data;
}				t_callback_info;

/**		Errors					*/
typedef enum	e_parser_error {
	PARSER_SUCCESS = 0,
	CALLBACK_EXIT,
	ERR_UNKNOW_OPTION,
	ERR_MISSING_VALUE,
	ERR_INVALID_FORMAT,
	ERR_OVERFLOW,
	ERR_MALLOC_FAILED,
}				t_parser_error;

typedef struct		s_parser_ctx {
	t_parser_error	err;
	const t_option	*opt;
	char			*token;
	char			*value;
}					t_parser_ctx;

/**		Help					*/

typedef struct			s_help_data {
	const t_parser_info	info;
	const t_option		*options;
}						t_help_data;

/**		Fonctions				*/
char	**parser(int argc, char **argv, const t_option *options, int mode, t_parser_ctx *ctx);
void	assign(const t_option *options, char *value, t_parser_ctx *ctx);
char	**append_arg(char **args, char *new_arg, t_parser_ctx *ctx);
void	cleaner(char **args);
void    debug(char **args, const t_option *options);
void	error(const char *program, t_parser_ctx *ctx);
void	callback_help(void *data);

#endif
