#ifndef __ULOG_H__
#define __ULOG_H__

#include <syslog.h>

void ulog_init(const char *ident);
void ulog(int priority, const char *message, ...);
void ulog_close(void);


#endif /* __ULOG_H__ */
