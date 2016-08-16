#include "log.h"
#include "string.h"
#include "zlog.h"

int logfd;
int logpro;
int debug_level;

zlog_category_t *zlog_handle = NULL;

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


int init_debug(const char *path){
    int rc = zlog_init(path);
    if(rc < 0){
        return -1;
    }
    zlog_handle = zlog_get_category("misaka");
    if(!zlog_handle){
        zlog_fini();
        return -2;
    }
    debug_level = DEBUG_ERR;
    zlog_info(zlog_handle, "misaka zlog init success");
    return 0;
}

void set_debug(int level){
    debug_level = level;
}

