/* Prints the argv the kernel handed us and returns argc as the exit code. */
#include <stdio.h>

int main(int argc, char **argv)
{
    printf("argtest: argc=%d\n", argc);
    for (int i = 0; i < argc; i++)
        printf("argtest: argv[%d]=[%s]\n", i, argv[i]);
    return argc;
}
