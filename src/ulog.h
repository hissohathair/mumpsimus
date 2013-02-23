#ifndef __ULOG_H__
#define __ULOG_H__

#include <stdarg.h>
#include <syslog.h>

#ifdef DEBUG
# define ulog_debug(...) ulog(LOG_DEBUG, __VA_ARGS__)
#else
# define ulog_debug(...)
#endif

void ulog_init(const char *ident);
void ulog(int priority, const char *message, ...);
void ulog_close(void);


#endif /* __ULOG_H__ */
