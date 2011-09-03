/**
 * Filename: debug.h
 * Description: Debugging support.
 */
/* vim:set cindent:set sw=4:set sts=4:set ts=4 */
#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

/*
 * Introduces the keyword 'debug' that executes code in a block of code if
 * NDEBUG is not set.
 */
#ifdef NDEBUG
#define debug \
	if (0)
#else
#define debug \
	if (1)
#endif

#endif /* DEBUG_H_INCLUDED */
