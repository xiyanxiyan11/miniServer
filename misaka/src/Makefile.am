AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -I$(top_srcdir)/include
bin_PROGRAMS = misaka
misaka_SOURCES = main.c
sUBDIRS = lib link task
misaka_LDFLAGS=-L$(top_srcdir)/lib -L$(top_srcdir)/task -L$(top_srcdir)/link 
LIBS= -lcore -llink -ltask -lpthread -lev -lm -g -O0



















