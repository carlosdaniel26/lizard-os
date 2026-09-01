/* Smoke test for SYS_read(fd 0): echo back each line the kernel hands us,
 * exit when the line starts with 'q'. */
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    char buf[128];
    printf("rdtest: type something\n");
    for (;;)
    {
        ssize_t n = read(0, buf, sizeof(buf) - 1);
        if (n <= 0)
            break;
        buf[n] = '\0';
        printf("rdtest: got %d bytes: [%s]\n", (int)n, buf);
        if (buf[0] == 'q')
            break;
    }
    printf("rdtest: bye\n");
    return 0;
}
