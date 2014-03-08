##
##  @(#)Makefile	1.00	Jan. 11, 2000
##
##  Makefile for syslogd/klogd
##  $Id: Makefile 7006 2010-05-12 16:28:54Z arun $

PROG=	syslogd
PIDFILE= -DPIDFILE=\"/var/run/syslogd.pid\"
CPPFLAGS+= -Ddebug=0 ${PIDFILE}
BINMODE= 700
MAN=

.include <minix.prog.mk>
