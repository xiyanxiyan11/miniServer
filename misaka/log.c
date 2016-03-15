#include "log.h"
#include "string.h"

int logfd;
int logpro;

int debug_flag;

/* Wrapper around strerror to handle case where it returns NULL. */
const char *
safe_strerror(int errnum)
{
  char *s = strerror(errnum);
  return (s != NULL) ? s : "Unknown error";
}

void zlog_backtrace(int i){
    fprintf(stderr, "zlog backtrace %d\n", i);
}
