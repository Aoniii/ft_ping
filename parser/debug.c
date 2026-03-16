#include "parser.h"
#include <stdio.h>
#include <stdbool.h>

void    debug(char **args, const t_option *options) {
    printf("--- OPTIONS ---\n");
    while (options) {
        if (options->flags == 0 && !options->short_opt && !options->long_opt)
            break;
        if (options->flags & OPT_CATEGORY) {
            options++;
            continue;
        }
        /**		Display the option name					*/
        if (options->long_opt)
            printf("--%-20s ", options->long_opt);
        else
            printf("-%-21c ", options->short_opt);

        printf("| Value: ");

        /**		Display the value based on the type		*/
        if (!options->value) {
            printf("(NULL pointer)\n");
        } else if (options->flags & TYPE_BOOLEAN) {
            printf("%s\n", *((bool *)options->value) ? "TRUE" : "FALSE");
        } else if (options->flags & TYPE_INT) {
            printf("%d\n", *((int *)options->value));
        } else if (options->flags & TYPE_UINT) {
            printf("%u\n", *((unsigned int *)options->value));
        } else if (options->flags & TYPE_DOUBLE) {
            printf("%f\n", *((double *)options->value));
        } else if (options->flags & TYPE_STRING) {
            printf("\"%s\"\n", *((char **)options->value) ? *((char **)options->value) : "NULL");
        } else if (options->flags & TYPE_COUNT) {
            printf("%d (count)\n", *((int *)options->value));
        } else if (options->flags & TYPE_CALLBACK) {
            printf("(callback function)\n");
        } else {
            printf("unknown type\n");
        }
        options++;
    }

    printf("\n--- POSITIONAL ARGUMENTS ---\n");
    if (!args || !args[0]) {
        printf("(none)\n");
    } else {
        for (int i = 0; args[i]; i++) {
            printf("Arg[%d]: %s\n", i, args[i]);
        }
    }
    printf("---------------------------\n");
}
