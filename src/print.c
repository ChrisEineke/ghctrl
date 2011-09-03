/**
 * Filename: print.c
 * Description: Definitions of printing helper functions.
 */
#include "print.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static FILE* stdlog = NULL;
static enum log_level log_level = LOG_NONE;

void set_log_fd(FILE* new_fd)
{
	stdlog = new_fd;
}

void set_log_level(enum log_level lvl)
{
	log_level = lvl;
}

#define VA_PRINT(format) \
	do { \
		va_list ap; \
		va_start(ap, format); \
		err_print(format, ap); \
		va_end(ap); \
	} while (0)

void err_print(const char *fmt, va_list ap)
{
	vfprintf(stdlog, fmt, ap);
	fflush(NULL);
}

void err_quit(const char *fmt, ...)
{
	VA_PRINT(fmt);
	exit(EXIT_FAILURE);
}

void _err_quitf(const char* func, unsigned line, const char *fmt, ...)
{
	fprintf(stdlog, "%s:%u: ", func, line);
	VA_PRINT(fmt);
	exit(EXIT_FAILURE);
}

#define DEFINE_SPECIAL_PRINT(_name, _log_level) \
	int _name(const char* fmt, ...) \
	{ \
		if (log_level >= _log_level) \
			VA_PRINT(fmt); \
		return 0; \
	}

DEFINE_SPECIAL_PRINT(print_info, LOG_INFO)
DEFINE_SPECIAL_PRINT(print_verbose, LOG_VERBOSE)
DEFINE_SPECIAL_PRINT(print_warn, LOG_WARN)
DEFINE_SPECIAL_PRINT(print_debug, LOG_DEBUG)
