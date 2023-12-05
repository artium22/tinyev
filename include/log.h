#ifndef log_h
#define log_h

#include <stdio.h>

#ifdef DEBUG
void sam_log_print(const char *file, const char *function, int line, const char *fmt, ...);

#define SLOG(fmt, ...)                                                      \
    do {                                                                    \
        sam_log_print(__FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);    \
    } while(0);
#else
#define SLOG(fmt, ...)
#endif

void log_init(char *path);

#endif /* log_h */
