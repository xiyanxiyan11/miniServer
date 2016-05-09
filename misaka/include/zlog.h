#ifndef __ZLOG_H__
#define __ZLOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

extern int debug_level;

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

#define TIME_INSTALL() time_t nowtime; struct tm *timeinfo; time( &nowtime ); timeinfo = localtime( &nowtime );

#define SET_DEBUG(level) do{debug_level = level; } while(0)
#define INIT_DEBUG()     do{debug_level = DEBUG_NONE; } while(0)

#ifdef DEBUG_VALUE

#define ZLOG_ERR(fmt, arg...)       do { if (DEBUG_ERR <= debug_level) { TIME_INSTALL() fprintf(stderr, DBG_FROMAT fmt, "E", \
								timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,\
								__FUNCTION__, __LINE__, ##arg);}} while(0)

#define ZLOG_WARN(fmt, arg...)      do { if (DEBUG_WARN <= debug_level) {TIME_INSTALL()  fprintf(stderr, DBG_FROMAT fmt, "W", \
								timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,\
								__FUNCTION__, __LINE__, ##arg);}} while(0)

#define ZLOG_INFO(fmt, arg...)      do { if (DEBUG_INFO <= debug_level) { TIME_INSTALL() fprintf(stderr, DBG_FROMAT fmt, "I", \
								timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,\
								__FUNCTION__, __LINE__, ##arg);}} while(0)

#define ZLOG_NOTICE(fmt, arg...)    do { if (DEBUG_NOTICE <= debug_level) { TIME_INSTALL() fprintf(stderr, DBG_FROMAT fmt, "N", \
								timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,\
								__FUNCTION__, __LINE__, ##arg);}} while(0)

#define ZLOG_DEBUG(fmt, arg...)     do { if (DEBUG_DEBUG <= debug_level) { TIME_INSTALL() fprintf(stderr, DBG_FROMAT fmt, "D", \
								timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,\
								__FUNCTION__, __LINE__, ##arg);}} while(0)

#define ZLOG(dst, fmt, arg...)      do { if (DEBUG_DEBUG <= debug_level) { TIME_INSTALL() fprintf(dst, DBG_FROMAT fmt, "A", \
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
