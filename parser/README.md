# Parser

A lightweight C library for parsing command-line arguments (`argc`/`argv`) with short options (`-x`), long options (`--option`), and positional arguments. It uses an array of option descriptors and supports multiple data types (boolean, integer, string, double, counter, callback).

---

## Table of Contents

- [Quick start (example: `main`)](#quick-start-example-main)
- [Key concepts](#key-concepts)
- [Parser API](#parser-api)
- [Option types and flags](#option-types-and-flags)
- [Parsing modes](#parsing-modes)
- [Error handling](#error-handling)
- [Utility functions](#utility-functions)
- [Project structure](#project-structure)

---

Examples using the sample executable:

```bash
./parser --bool -i 42 --double 3.14 file1.txt file2.txt
./parser -b -u 100 --string "hello" --help
./parser -- --force   # Everything after "--" is treated as a positional argument
```

---

## Quick start (example: `main`)

The `main.c` file serves as a complete example. Here are the steps for using the parser in your program.

### 1. Context and program information

```c
#include "parser.h"

t_parser_ctx  ctx;
ctx.err = PARSER_SUCCESS;

t_parser_info info = {
    .program     = argv[0],
    .usage       = "[Example...] Example ...",
    .description = "Description example"
};
```

- **`t_parser_ctx`**: holds the error code (`ctx.err`), the option that caused it (`ctx.opt`), the token (`ctx.token`), and the optional invalid value (`ctx.value`).
- **`t_parser_info`**: program name, usage line, and description for help (and `callback_help`).

### 2. Declare the variables that will hold the options

```c
bool         b = false;
int          i = 0;
unsigned int u = 0;
double       d = 0;
char        *s = NULL;
int          c = 0;
```

Each option points to one of these variables (or to a structure for callbacks).

### 3. Define the option array

The array is a sequence of `t_option` entries, ending with a sentinel entry `{0}`.

```c
t_option option[] = {
    CATEGORY("Category example:"),
    {
        .short_opt = 'b',
        .long_opt  = "bool",
        .flags     = OPT_SHORT | OPT_LONG | TYPE_BOOLEAN,
        .value     = &b,
        .help      = "Example of a boolean variable"
    },
    {
        .short_opt = 'i',
        .long_opt  = "int",
        .flags     = OPT_SHORT | OPT_LONG | TYPE_INT,
        .value     = &i,
        .help      = "Example of an integer variable"
    },
    // ... other options ...
    {
        .short_opt = 'h',
        .long_opt  = "help",
        .flags     = OPT_SHORT | OPT_LONG | OPT_CALLBACK_EXIT | OPT_HIDDEN_HELP | TYPE_CALLBACK,
        .value     = (void *)&(t_callback_info){
            .fn   = callback_help,
            .data = (void *)&(t_help_data){ .info = info, .options = option }
        },
        .help = "Example of a callback"
    },
    {0}   /* sentinel: end of table */
};
```

- **`CATEGORY("...")`**: title shown in help; no `.value`.
- **`.value`**: pointer to the variable (or to `t_callback_info` for a callback).
- **`.flags`**: combination of `OPT_SHORT`/`OPT_LONG`, a `TYPE_*` type, and optionally `OPT_CALLBACK_EXIT`, `OPT_HIDDEN_HELP`.

### 4. Run the parser

```c
char **args = parser(argc, argv, option, MODE_PERMISSIVE, &ctx);

if (ctx.err != PARSER_SUCCESS) {
    error(info.program, &ctx);
    cleaner(args);
    return (ctx.err == CALLBACK_EXIT ? 0 : 1);
}
```

- **`parser()`** returns a `NULL`-terminated array of strings containing the **positional arguments** (everything that is not an option). On error, the array may be partially filled; you must still free it with `cleaner(args)`.
- **`error()`** prints a message according to `ctx.err`.
- **`cleaner(args)`** frees the array and the strings allocated by the parser (or by `append_arg`).

### 5. Use the result

After a successful parse, the variables (`b`, `i`, `u`, `d`, `s`, `c`) are filled and `args` holds the positional arguments:

```c
debug(args, option);   /* example: print options + args */
cleaner(args);
return 0;
```

Flow summary in `main.c`: **context + info** → **variables + option array** → **parser()** → **check `ctx.err`** → **error() if needed** → **cleaner(args)** → **use options and `args`**.

---

## Key concepts

| Element | Role |
|--------|------|
| **Short options** | `-x`, `-i 42`, `-abc` (multiple letters grouped). |
| **Long options** | `--option`, `--option=value` or `--option value`. |
| **`--`** | Stops option parsing; the rest of the line is treated as positional arguments. |
| **Positional arguments** | Tokens that are not options; returned in the array from `parser()`. |
| **Sentinel** | Last entry in the option array: `{0}` (all fields zero). |

---

## Parser API

### `parser()`

```c
char **parser(int argc, char **argv, const t_option *options, int mode, t_parser_ctx *ctx);
```

- Walks `argv` and interprets each token according to `options`.
- **`mode`**: `MODE_STRICT` or `MODE_PERMISSIVE` (see [Parsing modes](#parsing-modes)).
- Fills `ctx` on error (`ctx->err`, `ctx->opt`, `ctx->token`, `ctx->value`).
- **Returns**: a heap-allocated, `NULL`-terminated array of positional arguments. Must be freed with `cleaner()`.

### `assign()`

Used internally to assign an option’s value to its target. You typically do not need to call it directly.

### `append_arg()`

```c
char **append_arg(char **args, char *new_arg, t_parser_ctx *ctx);
```

Appends `new_arg` to the array `args` (reallocates). On allocation failure, `ctx->err = ERR_MALLOC_FAILED`. Useful if you extend the parser or build your own argument list.

### `cleaner()`

```c
void cleaner(char **args);
```

Frees each element of `args` then the array. Call on the value returned by `parser()` (and on any array built with `append_arg()`).

### `error()`

```c
void error(const char *program, t_parser_ctx *ctx);
```

Prints an error message to stderr based on `ctx->err` (unknown option, missing value, invalid format, overflow, allocation).

### `callback_help()`

```c
void callback_help(void *data);
```

Ready-to-use callback to display help. `data` must be a `t_help_data*` containing `info` and `options`. Often used with a `--help` / `-h` option of type `TYPE_CALLBACK` and `OPT_CALLBACK_EXIT`.

---

## Option types and flags

### Option form (combine with a type)

| Flag | Meaning |
|------|--------|
| `OPT_SHORT` | Short option (`-x`). |
| `OPT_LONG` | Long option (`--option`). |
| `OPT_CALLBACK_EXIT` | After the callback, the parser stops and sets `ctx->err = CALLBACK_EXIT`. |
| `OPT_HIDDEN_HELP` | Option is not shown in the help generated by `callback_help`. |

### Value type (`TYPE_*`)

| Type | `.value` points to | Behavior |
|------|---------------------|----------|
| `TYPE_BOOLEAN` | `bool *` | Sets to `true` when the option is present (no value). |
| `TYPE_INT` | `int *` | Signed integer (base 10). Overflow handled by saturation (INT_MIN/INT_MAX). |
| `TYPE_UINT` | `unsigned int *` | Unsigned integer; negative value → error. |
| `TYPE_DOUBLE` | `double *` | Floating-point number. |
| `TYPE_STRING` | `char **` | Pointer to the given string (no copy). |
| `TYPE_COUNT` | `int *` | Increments the integer on each occurrence (e.g. `-vvv`). |
| `TYPE_CALLBACK` | `t_callback_info *` | Calls the function with the provided data. |

### Category (help only)

- **`CATEGORY("Title")`**: entry that only displays a title in help (`help` / `callback_help`). No `.value` and not a real option.

---

## Parsing modes

| Mode | Behavior |
|------|----------|
| **`MODE_STRICT`** | Once a positional argument is seen, the rest of the line is treated as positional (no more options after). |
| **`MODE_PERMISSIVE`** | Options and positional arguments can be interleaved; anything that is not an option is added to positional arguments. |

In both modes, the **`--`** token immediately stops option parsing.

---

## Error handling

Codes in `t_parser_ctx.err`:

| Code | Meaning |
|------|--------|
| `PARSER_SUCCESS` | Parsing OK. |
| `CALLBACK_EXIT` | A callback with `OPT_CALLBACK_EXIT` was run (e.g. `--help`); clean exit. |
| `ERR_UNKNOW_OPTION` | Unknown option (`ctx->token`). |
| `ERR_MISSING_VALUE` | Option that requires a value was used without one (`ctx->token`). |
| `ERR_INVALID_FORMAT` | Invalid value for the type (`ctx->value`). |
| `ERR_OVERFLOW` | Overflow (e.g. TYPE_COUNT counter). |
| `ERR_MALLOC_FAILED` | Allocation failed. |

In `main.c`, the recommended pattern is: if `ctx.err != PARSER_SUCCESS`, call `error(info.program, &ctx)`, then `cleaner(args)`, and return `0` for `CALLBACK_EXIT`, `1` otherwise.

---

## Utility functions

- **`debug(args, option)`** (provided in the example): prints options and their values plus positional arguments; handy for testing.
- **`callback_help`** + **`t_help_data`**: generate help from `t_parser_info` and the option array.

---

## Project structure

```
parser/
├── parser.h      Header: types, constants, declarations
├── parser.c      Parsing engine (short/long options, positionals)
├── assign.c      Value assignment by type (int, double, string, etc.)
├── append.c      Appending an argument to the array (append_arg)
├── cleaner.c     Freeing the argument array (cleaner)
├── error.c       Error message printing (error)
├── help.c        Help and callback_help
├── debug.c       Debug output (options + args)
├── main.c        Full usage example
├── source.mk     List of parser sources (excluding main.c)
└── Makefile      Build (parser + main)
```

To use the parser in another project: include `parser.h`, compile the sources listed in `source.mk` (excluding `main.c` and `debug.c` unless you want debug output), and link the executable with the resulting objects.

---

## Minimal example (without full `main.c`)

```c
#include "parser.h"

int main(int argc, char **argv) {
    t_parser_ctx ctx = { .err = PARSER_SUCCESS };
    bool verbose = false;
    t_option opts[] = {
        { .short_opt = 'v', .long_opt = "verbose",
          .flags = OPT_SHORT | OPT_LONG | TYPE_BOOLEAN, .value = &verbose, .help = "verbose" },
        {0}
    };

    char **args = parser(argc, argv, opts, MODE_PERMISSIVE, &ctx);
    if (ctx.err != PARSER_SUCCESS) {
        error(argv[0], &ctx);
        cleaner(args);
        return 1;
    }

    if (verbose)
        printf("Verbose mode\n");
    for (int i = 0; args && args[i]; i++)
        printf("arg[%d]: %s\n", i, args[i]);

    cleaner(args);
    return 0;
}
```

This README and the `main.c` example are enough to understand and reuse the parser in your own programs.
