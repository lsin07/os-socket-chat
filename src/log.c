#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void dbgMsg(const char *file, const int line, const char *func, const char *format, ...)
{
    printf("[DEBUG] ");
    printf("%s:%u: %s: ", file, line, func);
    va_list argList;
    va_start(argList, format);
    vprintf(format, argList);
    va_end(argList);
}

void dbgWarn(const char *file, const int line, const char *func, const char *format, ...)
{
    fprintf(stderr, "[DEBUG WARNING] ");
    fprintf(stderr, "%s:%u: %s: ", file, line, func);
    va_list argList;
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);
}

void sysErrExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void usrErrExit(const char *format, ...)
{
    fflush(stdout);
    fprintf(stderr, "fatal: ");
    va_list argList;

    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);

    fflush(stderr);
    exit(EXIT_FAILURE);
}

void dbgErrExit(const char *file, const int line, const char *func, const char *format, ...)
{
    fflush(stdout);
    fprintf(stderr, "[DEBUG FATAL] ");
    fprintf(stderr, "%s:%u: %s: ", file, line, func);

    va_list argList;
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);

    fflush(stderr);
    exit(EXIT_FAILURE);
}

void usageErr(const char *format, ...)
{
    fflush(stdout);
    va_list argList;

    fprintf(stderr, "Usage: ");
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);

    fflush(stderr);
    exit(EXIT_FAILURE);
}
