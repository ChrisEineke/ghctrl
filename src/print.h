/**
 * Filename: print.h
 * Description: Declarations of printing helper functions.
 */
#ifndef PRINT_H_INCLUDED
#define PRINT_H_INCLUDED

#include <stdio.h>

enum log_level {
	LOG_NONE,
	LOG_INFO,
	LOG_VERBOSE,
	LOG_WARN,
	LOG_DEBUG,
	LOG_ALL,
	LOG_END,
};

void set_log_fd(FILE*);
void set_log_level(enum log_level);

void err_quit(const char *fmt, ...) __attribute__((noreturn));
void _err_quitf(const char* func, unsigned line, const char *fmt, ...) __attribute__((noreturn));
#define err_quitf(fmt, ...) \
	do { \
		_err_quitf(__FUNCTION__, __LINE__, fmt, __VA_ARGS__); \
	} while (0);

int print_info(const char* fmt, ...);
int print_warn(const char* fmt, ...);
int print_verbose(const char* fmt, ...);
int print_debug(const char* fmt, ...);

#endif /* PRINT_H_INCLUDED */
