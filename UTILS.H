#ifndef __UTILS_H__
#define __UTILS_H__

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define HI_BYTE(x) (((x) >> 8) & 0xFF)
#define LO_BYTE(x) ((x) & 0xFF)

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

#define MEMBER_SIZE(type, member) (sizeof(((type *)0)->member))

#endif
