/* lizard's first userspace shell. Reads a line from the tty, splits it on
 * whitespace, runs a builtin or spawns an external program and waits for it.
 * There is no PATH search: an external command must name a path, so bare
 * "doom" is rejected and "./doom" (or "/doom", "../bin/doom") runs it,
 * resolved against the current directory the kernel tracks per task - "cd"
 * moves it. No pipes, redirection, globbing or job control yet - just the
 * essentials. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
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

/* An external command has to name a path - there is no PATH to search. Any
 * command word containing '/' qualifies ("./doom", "bin/doom", "/doom",
 * "../doom"); the kernel resolves it against the task's cwd on spawn. A bare
 * "doom" does not, and the shell rejects it with a hint. */
static int is_path(const char *cmd)
{
    return strchr(cmd, '/') != NULL;
}

int main(void)
{
    char line[LINE_MAX];
    char *argv[MAX_ARGS];
    char cwd[128];
    int last = 0;

    printf("lizard shell - 'help' for builtins\n");

    for (;;)
    {
        if (getcwd(cwd, sizeof(cwd)))
            printf("lizard:%s$ ", cwd);
        else
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
            printf("builtins: help  exit [code]  echo ...  clear  status  cd [dir]  pwd  ls [dir]\n");
            printf("else: ./prog or path/prog runs an external program (no PATH)\n");
            continue;
        }

        if (!strcmp(argv[0], "cd"))
        {
            const char *dir = argc > 1 ? argv[1] : "/";
            if (chdir(dir) != 0)
            {
                printf("cd: %s: no such directory\n", dir);
                last = 1;
            }
            else
                last = 0;
            continue;
        }

        if (!strcmp(argv[0], "pwd"))
        {
            if (getcwd(line, sizeof(line)))
                printf("%s\n", line);
            last = 0;
            continue;
        }

        if (!strcmp(argv[0], "ls"))
        {
            const char *dir = argc > 1 ? argv[1] : ".";
            int fd = sys_open(dir, 0 /* O_RDONLY */);
            if (fd < 0)
            {
                printf("ls: %s: cannot open\n", dir);
                last = 1;
                continue;
            }

            struct dirent ents[32];
            int total = 0, n;
            while ((n = sys_readdir(fd, ents, 32)) > 0)
            {
                for (int i = 0; i < n; i++)
                    printf("%s%s\n", ents[i].d_name,
                           ents[i].d_type == DT_DIR ? "/" : "");
                total += n;
            }
            sys_close(fd);
            last = n < 0 ? 1 : 0;
            (void)total;
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

        if (!is_path(argv[0]))
        {
            printf("%s: not found (use ./%s to run it from here)\n",
                   argv[0], argv[0]);
            last = 127;
            continue;
        }

        int pid = sys_spawn(argv[0], argv);
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
