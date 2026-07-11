# Manually created Makefile for zbxserver
CC = gcc
CFLAGS = -g -O2
CPPFLAGS = -DHAVE_CONFIG_H
INCLUDES = -I. -I../../../include -I../../../jainclude/

SRCS = evalfunc.c expression.c
OBJS = libzbxserver_a-evalfunc.o libzbxserver_a-expression.o

libzbxserver.a: 
	rm -f libzbxserver.a
	ar cru libzbxserver.a 
	ranlib libzbxserver.a

libzbxserver_a-%.o: %.c
	    -c -o  $<

.PHONY: clean
clean:
	rm -f  libzbxserver.a
