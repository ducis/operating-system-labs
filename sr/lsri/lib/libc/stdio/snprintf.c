/*
 * snprintf - print limited formatted output on an array
 */
/* $Header$ */

#include	<stdio.h>
#include	<stdarg.h>
#include	<limits.h>
#include	"loc_incl.h"

int
snprintf(char *s, size_t n, const char *format, ...)
{
	va_list ap;
	int retval;

	va_start(ap, format);

	retval = vsnprintf(s, n, format, ap);

	va_end(ap);

	return retval;
}
