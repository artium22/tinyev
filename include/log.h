#ifndef log_h
#define log_h

#include <stdio.h>

#ifdef DEBUG
void log_print(const char *file, const char *function, int line, const char *fmt, ...);

#define SLOG(fmt, ...)                                                      \
    do {                                                                    \
        log_print(__FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);        \
    } while(0);
#else
#define SLOG(fmt, ...)
#endif

void log_init(char *path);

void log_cleanup();

#endif /* log_h */
