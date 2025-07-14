#ifndef __UTILS_H__
#define __UTILS_H__

#define PERCENT_MIN 0L
#define PERCENT_MAX 100L

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MAP(x, xmin, xmax, ymin, ymax) (((x) - (xmin)) * ((ymax) - (ymin)) / ((xmax) - (xmin)) + (ymin))
#define CLAMP(x, lo, hi) MAX(lo, MIN(x, hi))

#define HI_BYTE(x) (((x) >> 8) & 0xFF)
#define LO_BYTE(x) ((x) & 0xFF)
#define BITS_TO_BYTES(x) (((x) + 7) / 8)

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))
#define MEMBER_SIZE(type, member) (sizeof(((type *)0)->member))

#endif
