#ifndef _UTILS_H
#define _UTILS_H

#ifndef _WIN32
#include <stddef.h>
typedef ptrdiff_t ssize_t;
#else
#include <unistd.h>
#endif

#ifdef __cplusplus
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stddef.h>
#else
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =============================================================== */
/* The following definitions create errors in C++ */
#define new(C) ((C *)calloc(sizeof(C), 1))
#define delete(O)                                                              \
	if ((O) != NULL) {                                                         \
		free((void *)(O));                                                     \
		(O) = NULL;                                                            \
	}
#define clamp(x, low, high)                                                    \
	((x) < (low) ? (low) : ((x) > (high) ? (high) : (x)))
#define SIGN(x) (((x) > 0) - ((x) < 0))
#endif

/* =============================================================== */
/* For interoperability with SDL2 */
typedef struct Point {
	int x;
	int y;
} Point;
typedef struct FPoint {
	float x;
	float y;
} FPoint;
typedef struct Rect {
	int x, y;
	int w, h;
} Rect;
typedef struct FRect {
	float x;
	float y;
	float w;
	float h;
} FRect;

/* =============================================================== */
/* Mathematics */
#undef M_E
#undef M_PI
#undef M_PI_2
#define M_E	   2.71828182845904523536028747135266249 /* e */
#define M_PI   3.14159265358979323846264338327950288 /* pi */
#define M_PI_2 1.57079632679489661923132169163975144 /* pi/2 */
#define M_2PI  6.28318530717958647692528676655900576 /* 2*pi */

#define M_TWO_THIRDS_PLUS1 1.666666

#define fequals_E(f1, f2, epsilon)	(fabs(f1 - f2) <= epsilon)
#define fequalsf_E(f1, f2, epsilon) (fabsf(f1 - f2) <= epsilon)
#define fequals(f1, f2)				fequals_E(f1, f2, DBL_EPSILON)
#define fequalsf(f1, f2)			fequalsf_E(f1, f2, FLT_EPSILON)

#define radtodeg(rad)			 (rad * (180.0 / M_PI))
#define degtorad(deg)			 (deg * (M_PI / 180.0))
#define clamp_high(x, high)		 ((x) > (high) ? (high) : (x))
#define clamp_low(x, low)		 ((x) < (low) ? (low) : (x))
#define lerp(a, b, t)			 ((1 - t) * a + t * b)
#define lengthdir_x(length, dir) (length * cos(dir))
#define lengthdir_y(length, dir) (length * sin(dir))

#define LERP_SPEED_VERY_SLOW  0.01
#define LERP_SPEED_SLOW		  0.03
#define LERP_SPEED_MEDIUM	  0.1
#define LERP_SPEED_QUICK	  0.4
#define LERP_SPEED_VERY_QUICK 0.7

/* =============================================================== */
/* MISC */
#ifndef UINT8_WIDTH
#define UINT8_WIDTH 8
#endif
#ifndef UINT16_WIDTH
#define UINT16_WIDTH 16
#endif
#ifndef UINT32_WIDTH
#define UINT32_WIDTH 32
#endif
#ifndef UINT64_WIDTH
#define UINT64_WIDTH 64
#endif

/** Big Endian single bitmask */
#define BIT(_i) (1 << (_i))

/** Little Endian single bitmask */
#define BITL(_bits, _i) (1 << (((_bits) - 1) - (_i)))

typedef uint8_t byte;
typedef int8_t	sbyte;
typedef struct _CList {
	size_t count;
	void **data;
} CList;

#define GRIDALIGN(p, s) ((p / s) * s)

#define SWAP(a, b)                                                             \
	{                                                                          \
		(a) ^= (b);                                                            \
		(b) ^= (a);                                                            \
		(a) ^= (b);                                                            \
	}

#define __repeat_body(n, v) for (size_t(v) = 0; (v) < (n); ++(v))

#define repeat(n) __repeat_body(n, _tmp##__COUNTER__##_)

/** Enable attribute packing in tcc compiler */
#if defined(__TINYC__)
#pragma pack(1)
#endif
#define PACKED __attribute__((packed))

#endif // _UTILS_H
