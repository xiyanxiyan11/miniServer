/* Memory management routine
   Copyright (C) 1998 Kunihiro Ishiguro

This file is part of GNU Zebra.

GNU Zebra is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

GNU Zebra is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Zebra; see the file COPYING.  If not, write to the Free
Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#ifndef _ZEBRA_MEMORY_H
#define _ZEBRA_MEMORY_H

/* For pretty printing of memory allocate information. */
struct memory_list
{
  int index;
  const char *format;
};

struct mlist {
  struct memory_list *list;
  const char *name;
};
 
#include "misaka.h"
#include "memtypes.h"

extern struct mlist mlists[];

void *misakamalloc(int mtype, int size);
void *misakacalloc(int mtype, int size);
void *misakarecalloc(int mtype, void *ptr, int size);
void misakafree(int mytype, void *ptr);


#define XMALLOC(mtype, size)       misakamalloc(mtype, size)
#define XCALLOC(mtype, size)       misakacalloc(mtype, size)
#define XREALLOC(mtype, ptr, size) misakarealloc(mtype, (ptr), (size))
#define XFREE(mtype, ptr)          do { \
                                        misakafree (mtype, ptr); \
                                        ptr = NULL; \
                                     } \
                                   while (0)

#define XSTRDUP(mtype, str)        strdup ((str))

#endif
