#include <stdarg.h>
#include <string.h>

#include "log.h"

#define LOG_MAX_LEN 512

static char buf[LOG_MAX_LEN + 1];
FILE *log_fp;

void log_print(const char *file, const char *function, int line, const char *fmt, ...)
{
    va_list argptr;
    int bytes;
    
    va_start(argptr, fmt);
    /* Prepare prefix. */
    bytes = sprintf(buf, "%s:%s:%d: ", strrchr(file, '/')+1, function, line);
    
    /* The content of the debug. */
    vsnprintf(buf + bytes, LOG_MAX_LEN - bytes, fmt, argptr);
    va_end(argptr);
    consolelog(buf);
}

void log_init(char *path)
{
    log_fp = fopen(path, "w");
}