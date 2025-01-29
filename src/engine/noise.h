#ifndef _NOISE_H
#define _NOISE_H

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "engine.h"

/** Set the seed for fast_rand */
void sfrand(size_t seed);

/** It's fine for low-entropy, non-critical applications where speed matters
 * more than statistical quality. */
size_t fast_rand();

/** fast_rand implementation for external seed manipulation */
size_t fast_rand_impl(size_t *seed);

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

/**
 * \brief Generate a pseudo-random noise for a specific location
 * \returns 0-255 number
 */
size_t noise2(size_t x, size_t y, seed_t SEED);

/**
 * \brief Generate a pseudo-random noise for a specific location
 * \details The noise is smoothed by a smooth interpolation of the four corners
 * of the grid.
 * \returns 0.0-255.0 number
 */
double noise2d(double x, double y, seed_t SEED);

/**
 * \brief Generate a pseudo-random Perlin noise for a specific location
 */
double perlin2d(seed_t SEED, double x, double y, double freq, size_t depth);

#endif // _NOISE_H
