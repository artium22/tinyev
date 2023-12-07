#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "log.h"

#define LOG_MAX_LEN 512

static char buf[LOG_MAX_LEN + 1];
FILE *log_fp = NULL;

void log_print(const char *file, const char *function, int line, const char *fmt, ...)
{
    va_list argptr;
    int bytes;
    
    /* Prepare prefix. */
    bytes = sprintf(buf, "%s:%s:%d: ", strrchr(file, '/')+1, function, line);
    
    /* The content of the debug. */
    va_start(argptr, fmt);
    vsnprintf(buf + bytes, LOG_MAX_LEN - bytes, fmt, argptr);
    va_end(argptr);
    fprintf(log_fp, "%s\n", buf);
    fflush(log_fp);
}

void log_init(char *path)
{
    char sec_path[16];
    int i = 0;

    log_fp = fopen(path, "w");
    while (!log_fp) {
        snprintf(sec_path, 16, "%s.%d", path, i++);
        log_fp = fopen(path, "w");
    }
}

void log_cleanup()
{
    fclose(log_fp);
}