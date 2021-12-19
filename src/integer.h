#ifndef UCRYPT_INTEGER_H
#define UCRYPT_INTEGER_H

#include <stdint.h>

/*
 * Each limb of a number has room for UC_DIGIT_BITS bits. The following must hold true:
 * - A uc_digit can hold UC_DIGIT_BITS + 1 bits
 * - A uc_word can hold 2 * UC_DIGIT_BITS + 1 bits
 *
 */

/*
 * The comba multiplication can be used if the number of digits in both factors does not exceed
 * UC_COMBA_MUL_MAX_DIGS. This value is calculated as the largest k that satisfies
 *      k < (2^alpha) / (beta^2 + 2*beta - 1)
 * where
 *  alpha number of bits in a word
 *  beta is the digit base
 */

//#define UC_DIGIT_8BIT
//#define UC_DIGIT_32BIT
#define UC_DIGIT_64BIT

#ifdef UC_DIGIT_8BIT
    typedef unsigned char uc_digit;
    typedef unsigned short uc_word;
    #define UC_DIGIT_BITS   7
    #define UC_COMBA_MUL_MAX_DIGS 4
#endif

#ifdef UC_DIGIT_32BIT
    typedef unsigned int    uc_digit;
    typedef unsigned long   uc_word;
    #define UC_DIGIT_BITS      28
    #define UC_COMBA_MUL_MAX_DIGS 255
#endif

#ifdef UC_DIGIT_64BIT
    typedef uint64_t        uc_digit;
    typedef __uint128_t     uc_word;
    #define UC_DIGIT_BITS   60
    #define UC_COMBA_MUL_MAX_DIGS 255
#endif

/*
 * Multi-precision integer data strcture.
 *
 * A uc_int variable _MUST_ be initialize with one of the uc_init_ function before use.
 */
typedef struct
{
    uc_digit *digits;   // array of digits in big endian
    int used;           // number of digits that are currently used
    int alloc;          // number of digits that are allocated (i.e., length of _digits_)
    int sign;           // UC_NEG or UC_POS
} uc_int;

/*
 * Constants
 */
#define UC_NEG 1
#define UC_POS 0

#define UC_LT -1
#define UC_EQ 0
#define UC_GT 1

#define UC_INT_BASE     (((uc_word) 1) << UC_DIGIT_BITS)
#define UC_DIGIT_MASK ((uc_digit) ~((~((uc_digit) 0)) << UC_DIGIT_BITS))

#define UC_OK               1       // Indicates that operation was successful
#define UC_INPUT_ERR        -1      // Indicates that operation was not successful
#define UC_MEM_ERR          -2      // Insufficient memory
#define UC_RNG_ERR          -3      // Random number generation error

#define UC_TRUE     1
#define UC_FALSE    0

/*
 * Basic operations
 */

int uc_init(uc_int *x);
int uc_init_multi(uc_int *x0, uc_int *x1, uc_int *x2, uc_int *x3, uc_int *x4, uc_int *x5);
int uc_init_zero(uc_int *x);
int uc_init_i(uc_int *x, int n);
int uc_init_l(uc_int *x, long n);
int uc_init_d(uc_int *x, uc_digit n);
int uc_init_w(uc_int *x, uc_word n);

int uc_set_zero(uc_int *x);
int uc_set_i(uc_int *x, int n);
int uc_set_l(uc_int *x, long n);
int uc_set_d(uc_int *x, uc_digit n);
int uc_set_w(uc_int *x, uc_word n);
int uc_copy(uc_int *x, uc_int *y);

int uc_grow(uc_int *x, int n);
int uc_clamp(uc_int *x);
int uc_free(uc_int *x);
int uc_free_multi(uc_int *x0, uc_int *x1, uc_int *x2, uc_int *x3, uc_int *x4, uc_int *x5);

/*
 * Comparisons
 */

int uc_cmp(uc_int *x, uc_int *y);
int uc_cmp_mag(uc_int *x, uc_int *y);

/* Signed comparisons */
#define uc_eq(x, y) (uc_cmp((x),(y)) == UC_EQ)      // ==
#define uc_neq(x, y) (uc_cmp((x),(y)) != UC_EQ)     // !=
#define uc_lt(x, y) (uc_cmp((x),(y)) == UC_LT)      // <
#define uc_lte(x, y) (uc_cmp((x),(y)) != UC_GT)     // <=
#define uc_gt(x, y) (uc_cmp((x),(y)) == UC_GT)      // >
#define uc_gte(x, y) (uc_cmp((x),(y)) != UC_LT)     // >=

/* Magnitude (i.e., unsigned) comparisons */
#define uc_eq_mag(x, y) (uc_cmp_mag((x),(y)) == UC_EQ)          // ==
#define uc_neq_mag(x, y) (uc_cmp_mag((x),(y)) != UC_EQ)         // !=
#define uc_lt_mag(x, y) (uc_cmp_mag((x),(y)) == UC_LT)          // <
#define uc_lte_mag(x, y) (uc_cmp_mag((x),(y)) != UC_GT)         // <=
#define uc_gt_mag(x, y) (uc_cmp_mag((x),(y)) == UC_GT)          // >
#define uc_gte_mag(x, y) (uc_cmp_mag((x),(y)) != UC_LT)         // >=

#define uc_is_zero(x) ((x)->used == 1 && (x)->digits[0] == 0)
#define uc_is_one(x) ((x)->used == 1 && (x)->digits[0] == 1)
#define uc_is_pos(x) ((x)->sign == UC_POS && !uc_is_zero(x))
#define uc_is_neg(x) ((x)->sign == UC_NEG)

#define uc_is_odd(x) ((x)->digits[0] & 1)
#define uc_is_even(x) (!uc_is_odd(x))

#define uc_div_ceil(x, y) ((x) / (y) + ((x) % (y) > 0))

/*
 * Integer arithmetic
 */

int uc_add(uc_int *z, uc_int *x, uc_int *y);
int uc_add_d(uc_int *z, uc_int *x, uc_digit d);
int uc_sub(uc_int *z, uc_int *x, uc_int *y);
int uc_sub_d(uc_int *z, uc_int *x, uc_digit y);
int uc_mul(uc_int *z, uc_int *x, uc_int *y);
int uc_mul_digs(uc_int *z, uc_int *x, uc_int *y, int digits);
int uc_mul_karatsuba(uc_int *z, uc_int *x, uc_int *y);
int uc_mul_2(uc_int *x, uc_int *y);
int uc_mul_d(uc_int *z, uc_int *x, uc_digit y);
int uc_mul_multi(uc_int *x, uc_int *ys, int k);
int uc_sqrd(uc_int *x, uc_int *y);
int uc_div(uc_int *q, uc_int *r, uc_int *x, uc_int *y);
int uc_div_school_small(uc_int *q, uc_int *r, uc_int *x, uc_int *y);
int uc_div_school_fast(uc_int *q, uc_int *r, uc_int *x, uc_int *y);
int uc_div_2(uc_int *x, uc_int *y);
int uc_div_d(uc_int *q, uc_digit *r, uc_int *x, uc_digit y);
int uc_exp(uc_int *z, uc_int *x, uc_int *y);
int uc_exp_i(uc_int *z, uc_int *x, int y);


int uc_lshb(uc_int *x, uc_int *y, int n);
int uc_rshb(uc_int *x, uc_int *y, int n);
int uc_lshd(uc_int *x, uc_int *y, int n);
int uc_rshd(uc_int *z, uc_int *x, int y);

int uc_abs(uc_int *x, uc_int *y);

int uc_flip_sign(uc_int *x);

int uc_count_bits(uc_int *x);
/* Returns n-th bit of x (index starts at 0) */
#define uc_nth_bit(x, n) (((x)->digits[n / UC_DIGIT_BITS] >> ((n) % UC_DIGIT_BITS)) & 1)

#define UC_MAG(x) ((x) >= 0 ? (x) : (-(x)))         /* |x| */
#define UC_MIN(x, y) ((x) <= (y) ? (x) : (y))       /* min(x, y) */
#define UC_MAX(x, y) ((x) >= (y) ? (x) : (y))       /* max(x, y) */

/*
 * Modular arithmetic
 */

int uc_add_mod(uc_int *z, uc_int *x, uc_int *y, uc_int *m);
int uc_mul_mod(uc_int *z, uc_int *x, uc_int *y, uc_int *m);
int uc_exp_mod(uc_int *z, uc_int *x, uc_int *y, uc_int *m);
int uc_mod_inv(uc_int *x, uc_int *y, uc_int *m);

int uc_mod(uc_int *x, uc_int *y, uc_int *m);
int uc_mod_base_pow(uc_int *x, uc_int *y, int k);

/*
 * Arithmetic algorithms
 */

int uc_gcd(uc_int *z, uc_int *x, uc_int *y);
uc_word uc_gcd_word(uc_word x, uc_word y);
int uc_egcd(uc_int *g, uc_int *u, uc_int *v, uc_int *a, uc_int *b);

int uc_lcm(uc_int *z, uc_int *x, uc_int *y);
uc_word uc_lcm_w(uc_word x, uc_word y);


int uc_int2rns(uc_int *xs, uc_int *x, uc_int *ms, int k);
int uc_rns2int(uc_int *x, uc_int *xs, uc_int *ms, int k);

/*
 * Conversion
 */

/* Read and write to and from string */
int uc_read_radix(uc_int *x, const char *y, int radix);
int uc_write_radix(char *s, uc_int *x, int radix);
int uc_write_radix_len(uc_int *x, int radix);

/* Read raw bytes */
int uc_read_bytes(uc_int *x, unsigned char *bytes, int nbytes);

/*
 * Miscellaneous / Debug
 */

void uc_debug_print_int(uc_int *x);
void uc_debug_print_int_bytes(uc_int *x);
void uc_debug_print_int_radix(uc_int *x, int radix);

#endif //UCRYPT_INTEGER_H
