#ifndef __ULOG_H__
#define __ULOG_H__

#include <stdarg.h>
#include <syslog.h>

#define ulog_debug(...) ulog(LOG_DEBUG, __VA_ARGS__)
#ifdef DEBUG
# define ulog_ping(x) ulog(LOG_DEBUG, "%s(%d): ping! (%s)", __FILE__, __LINE__, x)
#else
# define ulog_ping(x)
#endif

void ulog_init(const char *ident);
void ulog(int priority, const char *message, ...);
void ulog_close(void);


#endif /* __ULOG_H__ */
