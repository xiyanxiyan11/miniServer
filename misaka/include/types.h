#ifndef __PAD_TYPES_H__
#define __PAD_TYPES_H__

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


typedef signed char	int8;
typedef signed short 	int16;
typedef signed int 	int32;

typedef unsigned char 	uint8;
typedef unsigned short 	uint16;
typedef unsigned int 	uint32;

#ifndef MIN
#define MIN(a, b)	((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

#endif
