#include <lizard/boot.h>
#include <lizard/init.h>
#include <lizard/setup.h>
#include <nolibc/string.h>

#define MAX_CMDLINE_LEN 1024
__initdata char boot_cmdline[MAX_CMDLINE_LEN] = {0};

extern const struct setup_entry __setup_start[];
extern const struct setup_entry __setup_end[];

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

static int setup_params()
{
    strncpy(boot_cmdline, boot_info_ptr->cmdline, MAX_CMDLINE_LEN - 1);
    boot_cmdline[MAX_CMDLINE_LEN - 1] = '\0';

    char *cmdline = boot_cmdline;
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
        const struct setup_entry *ptr;
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

    return 0;
}

early_initcall(setup_params);