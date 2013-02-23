/*
 * ulog.c: Common logging functions for programs.
 *
 * Currently just using / supporting syslog.
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "ulog.h"

static int __logger_initialised = 0;

void ulog(int priority, const char *message, ...)
{
  va_list args;

  if ( ! __logger_initialised )
    ulog_init(__FILE__);

  va_start(args, message);
  vsyslog(priority, message, args);
  va_end(args);
}


void ulog_close(void)
{
  if ( __logger_initialised )
    closelog();
  __logger_initialised = 0;
  return;
}


#define ULOG_ENVVAR "ULOG_LEVEL"

void ulog_init(const char *ident)
{
  if ( __logger_initialised )
    return;

  /* logging PID and logging to stderr seems generally useful for now */
  openlog(ident, LOG_PID | LOG_PERROR, LOG_USER);

  int default_log_level = LOG_WARNING;
  if ( getenv(ULOG_ENVVAR) != NULL ) {
    char *endptr = NULL;
    long level = strtol( getenv(ULOG_ENVVAR), &endptr, 10 );
    if ( *endptr )
      fprintf(stderr, "%s: ERROR -- %s must be a number between 1 and 7 (default=%d; got '%s')\n", ident, ULOG_ENVVAR, 
	      default_log_level, getenv(ULOG_ENVVAR));
    else if ( (level < 0) || (level > 7) )
      fprintf(stderr, "%s: WARNING -- %s must be value between 1 and 7 (default=%d; got %ld)\n", ident, ULOG_ENVVAR,
	      default_log_level, level);
    else
      default_log_level = (int)level;
  }
  setlogmask( LOG_UPTO(default_log_level) );

  __logger_initialised = 1;
  atexit( ulog_close );

  return;
}

