#include <setup.h>
#include <string.h>

extern const struct SetupEntry __setup_start[];
extern const struct SetupEntry __setup_end[];

static inline char *skip_spaces(char *s)
{
    while (*s == ' ')
        s++;
    return s;
}

static inline char *next_arg(char *cmdline)
{
    while (*cmdline && *cmdline != ' ')
        cmdline++;

    return cmdline;
}

void parse_cmdline(char *cmdline)
{
    while (*cmdline)
    {
        cmdline = skip_spaces(cmdline);
        if (!*cmdline) break;

        char *arg = cmdline;
        char *end = next_arg(cmdline);

        if (*end)
        {
            *end = '\0';
            cmdline = end + 1;
        }
        else
        {
            cmdline = end;
        }

        /* all the corresponding function if the prefix matches. */
        SetupEntry *ptr;
        for (ptr = __setup_start; ptr < __setup_end; ptr++)
        {
            size_t len = strlen(ptr->str);

            if (!strncmp(arg, ptr->str, len))
            {
                char *val = arg + len;

                if (ptr->fn(val)) break;
            }
        }
    }
}