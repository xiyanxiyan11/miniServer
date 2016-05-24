#ifndef LOG_H__
#define LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <time.h>

extern int debug_level;

/* Message structure. */
struct message
{
  int key;
  const char *str;
};


#define DBG_FROMAT  "[%s %04d-%02d-%02d %02d:%02d:%02d %s:%d] "

enum DEBUG_LEVEL_ENUM{
    DEBUG_NONE = 0,
    DEBUG_ERR,
    DEBUG_WARN,
    DEBUG_INFO,
    DEBUG_NOTICE,
    DEBUG_DEBUG,
    DEBUG_MAX,
};


#define SET_DEBUG(level) do{debug_level = level; } while(0)
#define INIT_DEBUG()     do{debug_level = DEBUG_NONE; } while(0)

#if 1

#define zlog_err(fmt, arg...)       do { if (DEBUG_ERR <= debug_level) { \
                                                                time_t nowtime; struct tm *timeinfo; time( &nowtime ); timeinfo = localtime( &nowtime );\
                                                                fprintf(stderr, DBG_FROMAT fmt, "E", \
								timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,\
								__FUNCTION__, __LINE__, ##arg);}}while(0)

#define zlog_warn(fmt, arg...)      do { if (DEBUG_WARN <= debug_level) {\
                                                                time_t nowtime; struct tm *timeinfo; time( &nowtime ); timeinfo = localtime( &nowtime );\
                                                                fprintf(stderr, DBG_FROMAT fmt, "W", \
								timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,\
								__FUNCTION__, __LINE__, ##arg);}} while(0)

#define zlog_info(fmt, arg...)      do { if (DEBUG_INFO <= debug_level) { \
                                                                time_t nowtime; struct tm *timeinfo; time( &nowtime ); timeinfo = localtime( &nowtime );\
                                                                fprintf(stderr, DBG_FROMAT fmt, "I", \
								timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,\
								__FUNCTION__, __LINE__, ##arg);}} while(0)

#define zlog_notice(fmt, arg...)    do { if (DEBUG_NOTICE <= debug_level) { \
                                                                time_t nowtime; struct tm *timeinfo; time( &nowtime ); timeinfo = localtime( &nowtime );\
                                                                fprintf(stderr, DBG_FROMAT fmt, "N", \
								timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,\
								__FUNCTION__, __LINE__, ##arg);}} while(0)

#define zlog_debug(fmt, arg...)     do { if (DEBUG_DEBUG <= debug_level) { \
                                                                time_t nowtime; struct tm *timeinfo; time( &nowtime ); timeinfo = localtime( &nowtime );\
                                                                fprintf(stderr, DBG_FROMAT fmt, "D", \
								timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,\
								__FUNCTION__, __LINE__, ##arg);}} while(0)

#define zlog(dst, p, fmt, arg...)      do { if (DEBUG_DEBUG <= debug_level) {\
                                                                time_t nowtime; struct tm *timeinfo; time( &nowtime ); timeinfo = localtime( &nowtime );\
                                                                fprintf(dst, DBG_FROMAT fmt, "A", \
								timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,\
								__FUNCTION__, __LINE__, ##arg);}} while(0)


#define ZLOG_CHECK(level)           (level <= debug_level)

#else

#define ZLOG_CHECK()                (0)
#define ZLOG_ERR(fmt, arg...)      
#define ZLOG_WARN(fmt, arg...)      
#define ZLOG_INFO(fmt, arg...)       
#define ZLOG_NOTICE(fmt, arg...)     
#define ZLOG_DEBUG(fmt, arg...)      
#define ZLOG(fmt, arg...)            

#endif


#ifdef __cplusplus
}
#endif

#endif
