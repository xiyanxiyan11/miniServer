#ifndef _ZEBRA_STREAM_H
#define _ZEBRA_STREAM_H

#include "zebra.h"
#include "sockunion.h"

/* Stream buffer. */
struct stream
{
  struct stream *next;
  int owner;

  //union sockunion *su;  //src ip port
  //union sockunion *dsu; //dst ip port
    
  int type;             //packet type

  union sockunion su;	// Sockunion address of the peer
  union sockunion dsu;	// Sockunion address to connect

  int plen;             //payload of the buf
  int rlen;             //next read len;
  int flag;             //you can read some thing here
  int src;              //where are you from
  int dst;              //to where
  int hotplug;          //hot plug api, build by server, used to update api
  int sync;             //packet process sync or not?

  int mark;             //mark for user     

  /* Remainder is ***private*** to stream
   * direct access is frowned upon!
   * Use the appropriate functions/macros 
   */
  size_t getp; 		// next get position 
  size_t endp;		// last valid data position */
  size_t size;		// size of data segment 
  unsigned char *data;  // data pointer

  int sid;              //id of this session
  int seq;              //seq num
};

/* First in first out queue structure. */
struct stream_fifo
{
  size_t count;

  struct stream *head;
  struct stream *tail;
};

/* Utility macros. */
#define STREAM_SIZE(S)  ((S)->size)
  /* number of bytes which can still be written */
#define STREAM_WRITEABLE(S) ((S)->size - (S)->endp)
  /* number of bytes still to be read */
#define STREAM_READABLE(S) ((S)->endp - (S)->getp)

/* deprecated macros - do not use in new code */
#define STREAM_PNT(S)   stream_pnt((S))
#define STREAM_DATA(S)  ((S)->data)
#define STREAM_REMAIN(S) STREAM_WRITEABLE((S))

/* Stream prototypes. 
 * For stream_{put,get}S, the S suffix mean:
 *
 * c: character (unsigned byte)
 * w: word (two bytes)
 * l: long (two words)
 * q: quad (four words)
 */
extern struct stream *stream_new (size_t);
extern void stream_free (struct stream *);
extern struct stream * stream_copy (struct stream *, struct stream *src);
extern struct stream *stream_dup (struct stream *);
extern size_t stream_resize (struct stream *, size_t);
extern size_t stream_get_getp (struct stream *);
extern size_t stream_get_endp (struct stream *);
extern size_t stream_get_size (struct stream *);
extern u_char *stream_get_data (struct stream *);

extern void stream_set_getp (struct stream *, size_t);
extern void stream_forward_getp (struct stream *, size_t);
extern void stream_forward_endp (struct stream *, size_t);

/* steam_put: NULL source zeroes out size_t bytes of stream */
extern void stream_put (struct stream *, const void *, size_t);
extern int stream_putc (struct stream *, u_char);
extern int stream_putc_at (struct stream *, size_t, u_char);
extern int stream_putw (struct stream *, u16);
extern int stream_putw_at (struct stream *, size_t, u16);
extern int stream_putl (struct stream *, u32);
extern int stream_putl_at (struct stream *, size_t, u32);
extern int stream_putq (struct stream *, u64);
extern int stream_putq_at (struct stream *, size_t, u64);
extern int stream_put_ipv4 (struct stream *, u32);
extern int stream_put_in_addr (struct stream *, struct in_addr *);

extern void stream_get (void *, struct stream *, size_t);
extern u_char stream_getc (struct stream *);
extern u_char stream_getc_from (struct stream *, size_t);
extern u16 stream_getw (struct stream *);
extern u16 stream_getw_from (struct stream *, size_t);
extern u32 stream_getl (struct stream *);
extern u32 stream_getl_from (struct stream *, size_t);
extern u64 stream_getq (struct stream *);
extern u64 stream_getq_from (struct stream *, size_t);
extern u32 stream_get_ipv4 (struct stream *);

#undef stream_read
#undef stream_write

/* Deprecated: assumes blocking I/O.  Will be removed. 
   Use stream_read_try instead.  */
extern int stream_read (struct stream *, int, size_t);

/* Deprecated: all file descriptors should already be non-blocking.
   Will be removed.  Use stream_read_try instead. */
extern int stream_read_unblock (struct stream *, int, size_t);

/* Read up to size bytes into the stream.
   Return code:
     >0: number of bytes read
     0: end-of-file
     -1: fatal error
     -2: transient error, should retry later (i.e. EAGAIN or EINTR)
   This is suitable for use with non-blocking file descriptors.
 */
extern ssize_t stream_read_try(struct stream *s, int fd, size_t size);

extern ssize_t stream_recvmsg (struct stream *s, int fd, struct msghdr *,
                               int flags, size_t size);
extern ssize_t stream_recvfrom (struct stream *s, int fd, size_t len, 
                                int flags, struct sockaddr *from, 
                                socklen_t *fromlen);
extern size_t stream_write (struct stream *, const void *, size_t);

/* reset the stream. See Note above */
extern void stream_reset (struct stream *);
extern int stream_flush (struct stream *, int);
extern int stream_empty (struct stream *); /* is the stream empty? */

/* deprecated */
extern u_char *stream_pnt (struct stream *);

/* Stream fifo. */
extern struct stream_fifo *stream_fifo_new (void);
extern void stream_fifo_push (struct stream_fifo *fifo, struct stream *s);
extern void  stream_fifo_push_head(struct stream_fifo *fifo, struct stream *s);
extern struct stream *stream_fifo_pop (struct stream_fifo *fifo);
extern struct stream *stream_fifo_head (struct stream_fifo *fifo);
extern void stream_fifo_clean (struct stream_fifo *fifo);
extern void stream_fifo_free (struct stream_fifo *fifo);
extern void stream_forward ( struct stream *s, int size );

extern struct stream *stream_clone_one(struct stream *from);
extern struct stream * stream_clone(struct stream *from);
extern int stream_count(struct stream *s);
extern void stream_dir_exchange(struct stream *s);
extern void reverse_stream(struct stream *s, int size);
extern void dump_packet(struct stream *s);

#endif /* _ZEBRA_STREAM_H */
