#ifndef _NOISE_H
#define _NOISE_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "engine.h"

/** Set the seed for fast_rand */
void sfrand(size_t seed);

/** It's fine for low-entropy, non-critical applications where speed matters
 * more than statistical quality. */
size_t fast_rand();

/** Set the seed for mt_rand */
void mt_seed(size_t seed);

/**
 * \brief Mersenne Twister
 * \details Implementations generally create random numbers faster than
 * hardware-implemented methods. A study found that the Mersenne Twister creates
 * 64-bit floating point random numbers approximately twenty times faster than
 * the hardware-implemented, processor-based RDRAND instruction set
 */
size_t mt_rand();

double perlin2d(seed_t SEED, double x, double y, double freq, size_t depth);

#endif // _NOISE_H
