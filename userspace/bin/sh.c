/* lizard's first userspace shell. Reads a line from the tty, splits it on
 * whitespace, runs a builtin or spawns /<name> and waits for it. No pipes,
 * redirection, globbing or job control yet - just the essentials. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

#define LINE_MAX 256
#define MAX_ARGS 32

static int tokenize(char *line, char **argv)
{
    int argc = 0;
    char *p = line;

    while (*p && argc < MAX_ARGS - 1)
    {
        while (*p == ' ' || *p == '\t')
            p++;
        if (!*p)
            break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        if (*p)
            *p++ = '\0';
    }
    argv[argc] = NULL;
    return argc;
}

static int read_line(char *buf, int cap)
{
    int len = 0;
    while (len < cap - 1)
    {
        long n = read(0, buf + len, cap - 1 - len);
        if (n <= 0)
            return len ? len : -1; /* EOF with nothing buffered */
        len += (int)n;
        if (buf[len - 1] == '\n')
            break;
    }
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        len--;
    buf[len] = '\0';
    return len;
}

static void build_path(const char *cmd, char *path, int cap)
{
    if (cmd[0] == '/')
    {
        strncpy(path, cmd, cap - 1);
    }
    else
    {
        path[0] = '/';
        strncpy(path + 1, cmd, cap - 2);
    }
    path[cap - 1] = '\0';
}

int main(void)
{
    char line[LINE_MAX];
    char *argv[MAX_ARGS];
    char path[128];
    int last = 0;

    printf("lizard shell - 'help' for builtins\n");

    for (;;)
    {
        printf("lizard$ ");

        int len = read_line(line, sizeof(line));
        if (len < 0)
            return last; /* tty EOF */

        int argc = tokenize(line, argv);
        if (argc == 0)
            continue;

        if (!strcmp(argv[0], "exit"))
            return argc > 1 ? atoi(argv[1]) : last;

        if (!strcmp(argv[0], "help"))
        {
            printf("builtins: help  exit [code]  echo ...  clear  status\n");
            printf("anything else runs /<name> and waits for it\n");
            continue;
        }

        if (!strcmp(argv[0], "echo"))
        {
            for (int i = 1; i < argc; i++)
                printf("%s%s", argv[i], i + 1 < argc ? " " : "");
            printf("\n");
            continue;
        }

        if (!strcmp(argv[0], "clear"))
        {
            write(1, "\f", 1); /* the tty turns form-feed into a screen clear */
            continue;
        }

        if (!strcmp(argv[0], "status"))
        {
            printf("%d\n", last);
            continue;
        }

        build_path(argv[0], path, sizeof(path));

        int pid = sys_spawn(path, argv);
        if (pid < 0)
        {
            printf("%s: not found\n", argv[0]);
            last = 127;
            continue;
        }

        int status = 0;
        sys_waitpid(pid, &status, 0);
        last = status;
    }
}
