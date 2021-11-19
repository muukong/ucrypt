#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "integer.h"
#include "ucalloc.h"

static int _uc_add(uc_int *res, uc_int *x, uc_int *y);
static int _uc_mult(uc_int *res, uc_int *x, uc_int *y);

/*
 * Basic operations
 */

int uc_init(uc_int *x)
{
    if ( !x )
        return UC_INPUT_ERR;

    x->digits = NULL;
    x->alloc = 0;
    x->used = 0;
    x->sign = UC_POS;

    return UC_OK;
}

int uc_init_zero(uc_int *x)
{
    uc_init(x);
    uc_grow(x, 1);
    uc_zero_out(x);

    return UC_OK;
}

int uc_grow(uc_int *x, int n)
{
    assert( x && n >= 0 );

    int i;
    uc_digit *tmp;

    if (x->alloc < n )
    {
        tmp = XREALLOCARRAY(x->digits, n, sizeof(uc_digit));
        if ( !tmp )
            return UC_MEM_ERR;

        x->digits = tmp;

        // Zero out excess digits
        i = x->alloc;
        x->alloc = n;
        for ( ; i < x->alloc; ++i )
            x->digits[i] = 0;
    }

    return UC_OK;
}

int uc_init_from_int(uc_int *x, int n)
{
    return uc_init_from_long(x, n);
}

int uc_init_from_long(uc_int *x, long n)
{
    uc_init(x);
    uc_grow(x, sizeof(n) / sizeof(uc_digit) + 1);

    if ( n < 0 )
    {
        x->sign = UC_NEG;
        n *= -1; // TODO: can this overflow?
    }
    else
        x->sign = UC_POS;

    int digit_ctr = 0;
    uc_digit d = 0;
    for ( int i = 0; n > 0; ++i, n >>= 1 )
    {
        d += (n & 1) << (i % DIGIT_BITS);
        if ( (i + 1) % DIGIT_BITS == 0 )
        {
            x->digits[digit_ctr++] = d;
            x->used++;
            d = 0;
        }
    }

    // Add final digit
    x->digits[digit_ctr] = d;
    x->used++;

    // Remove trailing 0's. (TODO: check if this is needed)
    while ( x->digits[x->used-1] == 0 )
        --(x->used);

    return UC_OK;
}

/*
 * Convert byte-encoded integer _bytes_ (big-endian) of length _nbytes_ to integer
 */
int uc_init_from_bytes(uc_int *x, unsigned char *bytes, int nbytes)
{
    uc_init(x);
    uc_grow(x, (nbytes * 8) / DIGIT_BITS + 1);

    int digit_ctr = 0;
    uc_digit d = 0;
    for ( int i = 0; i < 8 * nbytes; ++i ) // iterate bit-wise over _bytes_
    {
        int b_i = (bytes[i/8] >> (i%8)) & 1;
        d += (b_i << (i % DIGIT_BITS));

        if ( (i + 1) % DIGIT_BITS == 0 ) // we have filled up a uc_digit (with DIGIT_BITS bits)
        {
            x->digits[digit_ctr++] = d;
            x->used++;
            d = 0;
        }
    }

    // Add final digit
    x->digits[digit_ctr] = d;
    x->used++;

    // Remove trailing 0's. (TODO: check if this is needed)
    while ( x->digits[x->used-1] == 0 )
        --(x->used);

    return UC_OK;
}

int uc_zero_out(uc_int *x)
{
    for ( int i = 0; i < x->used; ++i )
        x->digits[i] = 0;
    x->used = 1;
    x->sign = UC_POS;

    return UC_OK;
}

int uc_free(uc_int *x)
{
    uc_zero_out(x);     // make sure potentially sensitive data is erased from memory
    XFREE(x->digits);
    x->digits = NULL;
    x->used = 0;
    x->alloc = 0;
    x->sign = UC_POS;

    return UC_OK;
}

void debug_print(uc_int *x)
{
    int i;

    printf("UC Integer:\n");
    printf("Used  = %d\n", x->used);
    printf("Alloc = %d\n", x->alloc);
    printf("Sign = %d\n", x->sign);
    printf("Digits: [");
    for ( i = 0; i < x->alloc; ++i )
    {
        if ( i < x->used )
            printf("0x%02x, ", x->digits[i]);
        else
            printf("_%02x ", x->digits[i]);
    }
    printf("]\n");
}

/*
 * Comparisons
 */

/* Compare two integers (signed) */
int uc_cmp(uc_int *x, uc_int *y)
{
    if ( x->sign != y->sign )
    {
        if (x->sign == UC_NEG)
            return UC_LT;
        else
            return UC_GT;
    }

    if ( x->sign == UC_NEG )
        return uc_cmp_mag(y, x);
    else
        return uc_cmp_mag(x, y);
}

/*
 * Compare magnitude of two integers, i.e.,
 * |x| < |y| ==>  return UC_LT
 * |x| == |y| ==> return UC_EQ
 * |x| > |y| ==>  return UC_GT
 */
int uc_cmp_mag(uc_int *x, uc_int *y)
{
    if ( x->used < y->used )
        return UC_LT;
    if ( x->used > y->used )
        return UC_GT;

    for ( int i = x->used - 1; i >= 0; --i )
    {
        if ( x->digits[i] < y->digits[i] )
            return UC_LT;
        if ( x->digits[i] > y->digits[i] )
            return UC_GT;
    }

    return UC_EQ;
}

/*
 * Arithmetic operations
 */

int uc_add(uc_int *z, uc_int *x, uc_int *y)
{
    if ( uc_lt(x,y) )
    {
        printf("case 1");
        return _uc_add(z, y, x);
    }
    else
    {
        printf("case 2");
        return _uc_add(z, x, y);
    }
}

/*
 * Compute res = x * where where x >= y >= 0
 */
static int _uc_add(uc_int *res, uc_int *x, uc_int *y)
{
    int i;

    uc_grow(res, x->used + 1);

    uc_digit mask = (uc_digit) ~(((uc_digit) ~0) << DIGIT_BITS);

    uc_digit carry = 0;
    puts("");
    for ( i = 0; i < y->used; ++i )
    {
        uc_digit tmp = x->digits[i] + y->digits[i] + carry;
        carry = tmp >> DIGIT_BITS;
        res->digits[i] = tmp & mask;
        res->used++;
    }

    for ( ; i < x->used; ++i )
    {
        uc_digit tmp = x->digits[i] + carry;
        carry = tmp >> DIGIT_BITS;
        res->digits[i] = tmp & mask;
        res->used++;
    }

    if ( carry > 0 )
    {
        res->digits[i] = carry;
        res->used++;
    }

    return UC_OK;
}

int uc_mult(uc_int *z, uc_int *x, uc_int *y)
{
    if ( uc_lt(x, y) )
        return _uc_mult(z, y,x);
    else
        return _uc_mult(z, x,y);
}

/*
 * Compute res = x * where |x| >= |y|
 */
static int _uc_mult(uc_int *res, uc_int *x, uc_int *y)
{

}

int uc_flip_sign(uc_int *x)
{
    if ( uc_is_zero(x) ) // do nothing for zero (it's sign is always positive)
        return UC_OK;
}

