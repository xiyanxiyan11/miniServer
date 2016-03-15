#ifndef LOG_H
#define LOG_H

#include <stdio.h>

/* Message structure. */
struct message
{
  int key;
  const char *str;
};

extern const char * safe_strerror(int errnum);
void zlog_backtrace(int i);

extern int debug_flag;

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

#define SET_DEBUG(level) do{debug_flag = level; } while(0)
#define INIT_DEBUG()     do{debug_flag = DEBUG_NONE; } while(0)

#define zlog_err(fmt, arg...)       do { if (DEBUG_ERR <= debug_flag)       fprintf(stderr, DBG_FROMAT fmt " ", __FUNCTION__, __LINE__, ##arg);} while(0)
#define zlog_warn(fmt, arg...)      do { if (DEBUG_WARN <= debug_flag)      fprintf(stderr, DBG_FROMAT fmt " ", __FUNCTION__, __LINE__, ##arg);} while(0)
#define zlog_info(fmt, arg...)      do { if (DEBUG_INFO <= debug_flag)      fprintf(stderr, DBG_FROMAT fmt " ", __FUNCTION__, __LINE__, ##arg);} while(0)
#define zlog_notice(fmt, arg...)    do { if (DEBUG_NOTICE <= debug_flag)    fprintf(stderr, DBG_FROMAT fmt " ", __FUNCTION__, __LINE__, ##arg);} while(0)
#define zlog_debug(fmt, arg...)     do { if (DEBUG_DEBUG <= debug_flag)     fprintf(stderr, DBG_FROMAT fmt " ", __FUNCTION__, __LINE__, ##arg);} while(0)

//for some 3rd code
#define zlog(dst, p, fmt, arg...)   do {  fprintf(dst, DBG_FROMAT fmt, __FUNCTION__, __LINE__, ##arg);} while(0)

#endif
