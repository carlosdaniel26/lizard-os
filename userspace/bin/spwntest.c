/* Smoke test for SYS_spawn / SYS_waitpid: launch /hello a few times, collect
 * each exit code, then prove -ECHILD is returned once there are no children. */
#include <stdio.h>
#include <sys/syscall.h>

int main(void)
{
    for (int i = 0; i < 3; i++)
    {
        int pid = sys_spawn("/hello", 0);
        printf("spawntest: spawned /hello -> pid %d\n", pid);

        int status = -1;
        int w = sys_waitpid(pid, &status, 0);
        printf("spawntest: waitpid(%d) -> %d, status %d\n", pid, w, status);
    }

    int status = 0;
    int w = sys_waitpid(-1, &status, 0);
    printf("spawntest: waitpid(-1) with no children -> %d (want %d)\n", w, -10);

    return 0;
}
