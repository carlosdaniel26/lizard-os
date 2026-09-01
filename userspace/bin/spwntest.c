/* Smoke test for SYS_spawn / SYS_waitpid + argv passing: launch /hello a few
 * times, then /argtest with arguments, collecting each exit code; finally
 * prove -ECHILD once there are no children left. */
#include <stdio.h>
#include <sys/syscall.h>

int main(void)
{
    for (int i = 0; i < 2; i++)
    {
        int pid = sys_spawn("/hello", 0);
        int status = -1;
        int w = sys_waitpid(pid, &status, 0);
        printf("spwntest: /hello pid %d -> waitpid %d, status %d\n", pid, w, status);
    }

    char *av[] = {"argtest", "one", "two three", 0};
    int pid = sys_spawn("/argtest", av);
    int status = -1;
    int w = sys_waitpid(pid, &status, 0);
    printf("spwntest: /argtest pid %d -> waitpid %d, status %d (want 3)\n", pid, w, status);

    w = sys_waitpid(-1, &status, 0);
    printf("spwntest: waitpid(-1) no children -> %d (want -10)\n", w);

    return 0;
}
