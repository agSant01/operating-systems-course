#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void verror(const char *fmt, va_list argp)
{
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, argp);
    fprintf(stderr, "\n");
}

int main()
{

    verror("This is a test: %d , %d", {1, 2});

    return 0;
}