#ifndef _UTILS_H
#define _UTILS_H

#include <float.h>
#include <math.h>
#include <string.h>


/* =============================================================== */
/* MEMORY */
/* `calloc` allocates all elements initialized to 0 ,
 * required for NULL callbacks
 */
#define new(C) ((C *)calloc(sizeof(C), 1))
static void *memdup(const void *src, size_t size) {
	void *dst = malloc(size);
	if (!dst)
		return NULL;

	return memcpy(dst, src, size);
}


/* =============================================================== */
/* Mathematics */
#define M_2PI			   6.28318530717958647688
#define M_TWO_THIRDS_PLUS1 1.666666

#define fequals_E(f1, f2, epsilon)	(fabs(f1 - f2) <= epsilon)
#define fequalsf_E(f1, f2, epsilon) (fabsf(f1 - f2) <= epsilon)
#define fequals(f1, f2)				fequals_E(f1, f2, DBL_EPSILON)
#define fequalsf(f1, f2)			fequalsf_E(f1, f2, FLT_EPSILON)

#define radtodeg(rad)			 (rad * (180.0 / M_PI))
#define degtorad(deg)			 (deg * (M_PI / 180.0))
#define clamp(x, low, high)		 (x < low ? low : (x > high ? high : x))
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

typedef uint8_t byte;
typedef int8_t	sbyte;

#define GRIDALIGN(p, s) ((p / s) * block_size)

#define SWAP(a, b)                                                             \
	{                                                                          \
		a ^= b;                                                                \
		b ^= a;                                                                \
		a ^= b;                                                                \
	}

/**
 * Cross-platform sleep function for C
 * @param int milliseconds
 */
#ifdef WIN32
#include <windows.h>
#define sleep(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep(ms) usleep(ms * 1000)
#endif

/** Enable attribute packing in tcc compiler */
#if defined(__TINYC__)
#pragma pack(1)
#endif
#define PACKED __attribute__((packed))

#endif // _UTILS_H
