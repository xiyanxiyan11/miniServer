AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -I$(top_srcdir)/include
bin_PROGRAMS = echoserver
echoserver_SOURCES = main.c
echoserver_LDFLAGS=-L$(top_srcdir)/lib -L$(top_srcdir)/task -L$(top_srcdir)/link 
LIBS= -lcore -llink -lpthread -lev -lm -lzlog  -ldl  -llua5.1 -lpython2.7 -g -O0 



















