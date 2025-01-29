#include "noise.h"

/* ========================================================================= */
/* Fast Random */

size_t fast_rand_impl(size_t *seed) {
#if __SIZE_WIDTH__ == 64
	*seed ^= *seed << 7;
	*seed ^= *seed >> 9;
#else
	*seed ^= *seed << 13;
	*seed ^= *seed >> 17;
	*seed ^= *seed << 5;
#endif
	return *seed;
}

static size_t _fseed_ = SIZE_MAX;

void   sfrand(size_t seed) { _fseed_ = seed; }
size_t fast_rand() { return fast_rand_impl(&_fseed_); }

/* ========================================================================= */
/* Mersenne Twister Random */

#define MT_N 312
#define MT_M 156

#define MT_LOWER_MASK (SIZE_MAX / 2) /* least significant r bits */
#define MT_UPPER_MASK                                                          \
	(SIZE_MAX & ~MT_LOWER_MASK) /* most significant w-r                        \
								   bits*/

/* It is a 64-bit hexadecimal constant, often referred to as matrix A in
Mersenne Twister (MT19937-64). This constant is crucial for the mixing steps
in the random number generation process. Its binary form is
1011010100000001101111100101101010101001100110000110011110011110, which has
been empirically chosen to improve the quality of randomness. */
#define MT_MATRIX_A (0xB5026F5AA96619E9ULL) /* constant vector a */

/* The value has been carefully chosen to distribute seed values evenly
 * throughout the internal state space, reducing correlations between
 successive states. */
#define SEED_DISTRIBUTOR (6364136223846793005ULL)

/* The default seed value 5489ULL is commonly used in the Mersenne Twister
 * implementation because it is a well-tested, pre-selected seed that has
 been
 * chosen for its statistical properties and consistent results. */
#define SEED_DEFAULT (5489ULL)

static size_t mt[MT_N]; /* the array for the state vector */
static size_t mt_index =
	MT_N + 1; /* mt_index == MT_N+1 indicates uninitialized */

void mt_seed(size_t seed) {
	mt[0] = seed;
	for (mt_index = 1; mt_index < MT_N; mt_index++) {
		mt[mt_index] =
			(SEED_DISTRIBUTOR * (mt[mt_index - 1] ^ (mt[mt_index - 1] >> 62)) +
			 mt_index);
	}
}

size_t mt_rand() {
	size_t y;

	static const size_t mag[2] = {0, MT_MATRIX_A};

	if (mt_index >= MT_N) { /* Generate MT_N words at a time */
		size_t i;

		if (mt_index == MT_N + 1)  /* Seed not initialized */
			mt_seed(SEED_DEFAULT); /* Default seed */

		for (i = 0; i < MT_N - MT_M; i++) {
			y	  = (mt[i] & MT_UPPER_MASK) | (mt[i + 1] & MT_LOWER_MASK);
			mt[i] = mt[i + MT_M] ^ (y >> 1) ^ mag[y & 1];
		}
		for (; i < MT_N - 1; i++) {
			y	  = (mt[i] & MT_UPPER_MASK) | (mt[i + 1] & MT_LOWER_MASK);
			mt[i] = mt[i + (MT_M - MT_N)] ^ (y >> 1) ^ mag[y & 1];
		}
		y			 = (mt[MT_N - 1] & MT_UPPER_MASK) | (mt[0] & MT_LOWER_MASK);
		mt[MT_N - 1] = mt[MT_M - 1] ^ (y >> 1) ^ mag[y & 1];

		mt_index = 0;
	}

	y = mt[mt_index++];

	/* The "magic numbers" in the tempering step of the Mersenne Twister
	 * algorithm are specifically chosen bitwise constants that help to further
	 * scramble the raw random values produced by the state transition,
	 * improving the randomness and statistical properties of the generated
	 * numbers. The purpose of tempering is to reduce any biases that might have
	 * been introduced during the state transitions and make the random number
	 * output more uniform and unpredictable. */

	/* Tempering */
	y ^= (y >> 29) & ((size_t)0x5555555555555555);
	y ^= (y << 17) & ((size_t)0x71D67FFFEDA60000);
	y ^= (y << 37) & ((size_t)0xFFF7EEE000000000);
	y ^= (y >> 43);

	return y;
}

/* ========================================================================= */
/* Perlin Noise */
static const unsigned char HASH[] = {
	208, 34,  231, 213, 32,	 248, 233, 56,	161, 78,  24,  140, 71,	 48,  140,
	254, 245, 255, 247, 247, 40,  185, 248, 251, 245, 28,  124, 204, 204, 76,
	36,	 1,	  107, 28,	234, 163, 202, 224, 245, 128, 167, 204, 9,	 92,  217,
	54,	 239, 174, 173, 102, 193, 189, 190, 121, 100, 108, 167, 44,	 43,  77,
	180, 204, 8,   81,	70,	 223, 11,  38,	24,	 254, 210, 210, 177, 32,  81,
	195, 243, 125, 8,	169, 112, 32,  97,	53,	 195, 13,  203, 9,	 47,  104,
	125, 117, 114, 124, 165, 203, 181, 235, 193, 206, 70,  180, 174, 0,	  167,
	181, 41,  164, 30,	116, 127, 198, 245, 146, 87,  224, 149, 206, 57,  4,
	192, 210, 65,  210, 129, 240, 178, 105, 228, 108, 245, 148, 140, 40,  35,
	195, 38,  58,  65,	207, 215, 253, 65,	85,	 208, 76,  62,	3,	 237, 55,
	89,	 232, 50,  217, 64,	 244, 157, 199, 121, 252, 90,  17,	212, 203, 149,
	152, 140, 187, 234, 177, 73,  174, 193, 100, 192, 143, 97,	53,	 145, 135,
	19,	 103, 13,  90,	135, 151, 199, 91,	239, 247, 33,  39,	145, 101, 120,
	99,	 3,	  186, 86,	99,	 41,  237, 203, 111, 79,  220, 135, 158, 42,  30,
	154, 120, 67,  87,	167, 135, 176, 183, 191, 253, 115, 184, 21,	 233, 58,
	129, 233, 142, 39,	128, 211, 118, 137, 139, 255, 114, 20,	218, 113, 154,
	27,	 127, 246, 250, 1,	 8,	  198, 250, 209, 92,  222, 173, 21,	 88,  102,
	219};

size_t noise2(size_t x, size_t y, seed_t SEED) {
	size_t yindex = (y + SEED) % 256;
	if (yindex < 0)
		yindex += 256;
	size_t xindex = (HASH[yindex] + x) % 256;
	if (xindex < 0)
		xindex += 256;
	const size_t result = HASH[xindex];
	return result;
}

static double lin_inter(double x, double y, double s) {
	return x + s * (y - x);
}

static double smooth_inter(double x, double y, double s) {
	return lin_inter(x, y, s * s * (3 - 2 * s));
}

double noise2d(double x, double y, seed_t SEED) {
	const size_t x_int	= floor(x);
	const size_t y_int	= floor(y);
	const double x_frac = x - x_int;
	const double y_frac = y - y_int;
	const size_t s		= noise2(x_int, y_int, SEED);
	const size_t t		= noise2(x_int + 1, y_int, SEED);
	const size_t u		= noise2(x_int, y_int + 1, SEED);
	const size_t v		= noise2(x_int + 1, y_int + 1, SEED);
	const double low	= smooth_inter(s, t, x_frac);
	const double high	= smooth_inter(u, v, x_frac);
	const double result = smooth_inter(low, high, y_frac);
	return result;
}

double perlin2d(seed_t SEED, double x, double y, double freq, size_t depth) {
	double xa  = x * freq;
	double ya  = y * freq;
	double amp = 1.0;
	double fin = 0;
	double div = 0.0;
	for (size_t i = 0; i < depth; i++) {
		div += 256 * amp;
		fin += noise2d(xa, ya, SEED) * amp;
		amp /= 2;
		xa *= 2;
		ya *= 2;
	}
	return fin / div;
}
