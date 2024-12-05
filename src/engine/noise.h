#ifndef _NOISE_H
#define _NOISE_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "engine.h"

double perlin2d(seed_t SEED, double x, double y, double freq, size_t depth);

#endif // _NOISE_H
