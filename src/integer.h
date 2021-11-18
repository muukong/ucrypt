#ifndef UCRYPT_INTEGER_H
#define UCRYPT_INTEGER_H

/*
 * Constants
 */
#define UC_NEG 1
#define UC_POS 0

#define UC_OK               1       // Indicates that operation was successful
#define UC_INPUT_ERR        -1      // Indicates that operation was not successful
#define UC_MEM_ERR          -2      // Insufficient memory

/*
 * Each limb of a number has room for 31 bits. The following must hold true:
 * - A uc_digit can hold DIGIT_BITS + 1 bits
 * - A uc_word can hold 2 * DIGIT_BITS + 1 bits
 */

/*
#define DIGIT_BITS      7
typedef unsigned char uc_digit;
typedef unsigned short uc_word;
 */

#define DIGIT_BITS      31
typedef unsigned int    uc_digit;
typedef unsigned long   uc_word;

typedef struct
{
    uc_digit *digits;   // array of digits in big endian
    int used;           // number of digits that are currently used
    int alloc;          // number of digits that are allocated (i.e., length of _digits_)
    int sign;           // UC_NEG or UC_POS
} uc_int;

/* Simple inline operations */




/*
 * Basic operations
 */

int uc_init(uc_int *x);

int uc_grow(uc_int *x, int n);

int uc_init_from_int(uc_int *x, int n);
int uc_init_from_long(uc_int *x, long n);
int uc_init_from_bytes(uc_int *x, unsigned char *bytes, int nbytes);

void debug_print(uc_int *x);

#endif //UCRYPT_INTEGER_H
