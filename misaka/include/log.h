#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <zlog.h>

/* Message structure. */
struct message
{
  int key;
  const char *str;
};

extern const char * safe_strerror(int errnum);
void zlog_backtrace(int i);

extern int debug_level;

#define DBG_FROMAT  "DBG[%s]:[%d]>>"

enum DEBUG_LEVEL{
    DEBUG_NONE = 0,
    DEBUG_ERR,
    DEBUG_WARN,
    DEBUG_INFO,
    DEBUG_NOTICE,
    DEBUG_DEBUG,
    DEBUG_MAX,
};

extern zlog_category_t *zlog_handle;
extern int init_debug(const char *path);
extern void set_debug(int level);

#define SET_DEBUG(level) set_debug(level)
#define INIT_DEBUG(path) init_debug(path)

#define mlog_err(fmt, arg...)       do { if (DEBUG_ERR <= debug_level)       zlog_error(zlog_handle, DBG_FROMAT fmt " ", __FUNCTION__, __LINE__, ##arg);} while(0)
#define mlog_warn(fmt, arg...)      do { if (DEBUG_WARN <= debug_level)      zlog_warn(zlog_handle, DBG_FROMAT fmt " ", __FUNCTION__, __LINE__, ##arg);} while(0)
#define mlog_info(fmt, arg...)      do { if (DEBUG_INFO <= debug_level)      zlog_info(zlog_handle, DBG_FROMAT fmt " ", __FUNCTION__, __LINE__, ##arg);} while(0)
#define mlog_notice(fmt, arg...)    do { if (DEBUG_NOTICE <= debug_level)    zlog_notice(zlog_handle, DBG_FROMAT fmt " ", __FUNCTION__, __LINE__, ##arg);} while(0)
#define mlog_debug(fmt, arg...)     do { if (DEBUG_DEBUG <= debug_level)     zlog_debug(zlog_handle, DBG_FROMAT fmt " ", __FUNCTION__, __LINE__, ##arg);} while(0)

//for some 3rd code
#define mlog(dst, p, fmt, arg...)   do {  zlog_debug(zlog_handle, DBG_FROMAT fmt, __FUNCTION__, __LINE__, ##arg);} while(0)

#endif
