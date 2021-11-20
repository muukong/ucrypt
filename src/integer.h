#ifndef UCRYPT_INTEGER_H
#define UCRYPT_INTEGER_H

/*
 * Constants
 */
#define UC_NEG 1
#define UC_POS 0

#define UC_LT -1
#define UC_EQ 0
#define UC_GT 1

#define UC_DIGIT_MASK ((uc_digit) ~(((uc_digit) ~0) << DIGIT_BITS))

#define UC_OK               1       // Indicates that operation was successful
#define UC_INPUT_ERR        -1      // Indicates that operation was not successful
#define UC_MEM_ERR          -2      // Insufficient memory

/*
 * Each limb of a number has room for 31 bits. The following must hold true:
 * - A uc_digit can hold DIGIT_BITS + 1 bits
 * - A uc_word can hold 2 * DIGIT_BITS + 1 bits
 */

#define DIGIT_BITS      7
typedef unsigned char uc_digit;
typedef unsigned short uc_word;

/*
#define DIGIT_BITS      31
typedef unsigned int    uc_digit;
typedef unsigned long   uc_word;
 */

typedef struct
{
    uc_digit *digits;   // array of digits in big endian
    int used;           // number of digits that are currently used
    int alloc;          // number of digits that are allocated (i.e., length of _digits_)
    int sign;           // UC_NEG or UC_POS
} uc_int;

/*
 * Basic operations
 */

int uc_init(uc_int *x);
int uc_init_zero(uc_int *x);
int uc_init_from_int(uc_int *x, int n);
int uc_init_from_long(uc_int *x, long n);
int uc_init_from_bytes(uc_int *x, unsigned char *bytes, int nbytes);

int uc_zero_out(uc_int *x);
int uc_free(uc_int *x);

int uc_grow(uc_int *x, int n);

int uc_clamp(uc_int *x);

/*
 * Comparisons
 */

int uc_cmp(uc_int *x, uc_int *y);
int uc_cmp_mag(uc_int *x, uc_int *y);

#define uc_eq(x, y) (uc_cmp((x),(y)) == UC_EQ)      // ==
#define uc_neq(x, y) (uc_cmp((x),(y)) != UC_EQ)     // !=
#define uc_lt(x, y) (uc_cmp((x),(y)) == UC_LT)      // <
#define uc_lte(x, y) (uc_cmp((x),(y)) != UC_GT)     // <=
#define uc_gt(x, y) (uc_cmp((x),(y)) == UC_GT)      // >
#define uc_gte(x, y) (uc_cmp((x),(y)) != UC_LT)     // >=

#define uc_is_zero(x) ((x)->used == 1 && (x)->digits[0] == 0)
#define uc_is_pos(x) ((x)->sign == UC_NEG)
#define uc_is_neg(x) ((x)->sign == UC_NEG)

#define uc_is_odd(x) ((x)->digits[0] & 1)
#define uc_is_even(x) (!uc_is_odd(x))

/*
 * Arithmetic operations
 */

int uc_add(uc_int *z, uc_int *x, uc_int *y);
int uc_mul(uc_int *z, uc_int *x, uc_int *y);

int uc_flip_sign(uc_int *x);

int uc_gc(uc_int *z, uc_int *x, uc_int *y);

/*
 * Miscellaneous / Debug
 */

void debug_print(uc_int *x);

#endif //UCRYPT_INTEGER_H
