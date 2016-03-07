#ifndef LIBY_LOG_H
#define LIBY_LOG_H

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define MAXLINE (4096)

void log_err(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_exit(int err, const char *fmt, ...);
static void log_doit(int errnoflag, int error, const char *fmt, va_list ap);
void log_quit(const char *fmt, ...);

#endif
