#define _GNU_SOURCE
#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void log_msg(const char *fmt, ...)
{
    va_list ap;
    char buf[1024];
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(buf, sizeof(buf), "%F %T", &tm);
    fprintf(stdout, "[%s] ", buf);
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fprintf(stdout, "\n");
    fflush(stdout);
}