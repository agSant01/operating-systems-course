#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int v = 0;
int a()
{
    return v++;
}

int main()
{
    // printf("This is var: %s\n", strcat("hello", "advav"));

    int r = 1;

    r = 'a' == 'a';

    printf("RE: %d\n", r);
}