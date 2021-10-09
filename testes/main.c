#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    printf("argc: %d\n", argc);
    printf("argc: %d\n", STDIN_FILENO);

    return 0;
}