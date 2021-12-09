#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "integer.h"
#include "string_util.h"
#include "ucalloc.h"

/*
 * Helper functions that are only used internally
 */

static int _uc_add(uc_int *z, uc_int *x, uc_int *y);
static int _uc_sub(uc_int *z, uc_int *x, uc_int *y);
static int _uc_mul(uc_int *z, uc_int *x, uc_int *y);
static int _uc_div(uc_int *q, uc_int *r, uc_int *x, uc_int *y);

int _uc_write_radix_slow(char *s, int *n, uc_int *x, int radix);

static int _check_div(uc_int *q, uc_int *r, uc_int *x, uc_int *y);

static uc_word _uc_gcd_word(uc_word x, uc_word y);

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

/*
 * Initializes multiple UC integers. If fewer integers need to be initialized, the spare
 * parameters on the right can be set to NULL and won't be initialized (e.g.
 * uc_init_multi(&a, &b, &c, NULL, NULL, NULL));
 */
int uc_init_multi(uc_int *x0, uc_int *x1, uc_int *x2, uc_int *x3, uc_int *x4, uc_int *x5)
{
    int ret;

    ret = UC_OK;

    if ( x0 && ((ret = uc_init(x0)) != UC_OK) )
        return ret;

    if ( x1 && ((ret = uc_init(x1)) != UC_OK) )
    {
        uc_free(x0);
        return ret;
    }

    if ( x2 && ((ret = uc_init(x2)) != UC_OK) )
    {
        uc_free(x0);
        uc_free(x1);
        return ret;
    }

    if ( x3 && ((ret = uc_init(x3)) != UC_OK) )
    {
        uc_free(x0);
        uc_free(x1);
        uc_free(x2);
        return ret;
    }

    if ( x4 && ((ret = uc_init(x4)) != UC_OK) )
    {
        uc_free(x0);
        uc_free(x1);
        uc_free(x2);
        uc_free(x3);
        return ret;
    }

    if ( x5 && ((ret = uc_init(x5)) != UC_OK) )
    {
        uc_free(x0);
        uc_free(x1);
        uc_free(x2);
        uc_free(x3);
        uc_free(x4);
        return ret;
    }

    return ret;
}

int uc_init_zero(uc_int *x)
{
    uc_init(x);
    return uc_set_zero(x);
}

/*
 * Grow integer x s.t. it has at least n digits allocated.
 *
 * For example, growing from k to n digits (n > k):
 * [b_0 b_1 b_2 .. b_{k-1}] ---grow---> [b_0 b_1 b_2 .. b_k 0 0 ... 0 0 0]
 *      ^          ^                         ^                          ^
 *      |          |                         |                          |
 *      x_used     k-th digit                x_used                     n-th digit
 *
 * If x already has at least n digits, we do nothing.
 */
int uc_grow(uc_int *x, int n)
{
    assert( x );
    assert( n >= 1 );

    int i;
    uc_digit *tmp;

    if (x->alloc < n )
    {
        tmp = XREALLOCARRAY(x->digits, n, sizeof(uc_digit));
        if ( !tmp )
            return UC_MEM_ERR;

        x->digits = tmp;

        /*
         * Growing an integer does not change x->used for non-zero numbers. However,
         * if x->digits was NULL (i.e., x->used == 0) we must increase x->used
         * to 1.
         */
        if ( x->used == 0 )
            x->used = 1;

        // Zero out excess digits
        i = x->alloc;
        x->alloc = n;
        for ( ; i < x->alloc; ++i )
            x->digits[i] = 0;
    }

    return UC_OK;
}

/*
 * Initialize UC integer x with built-in int n.
 */
int uc_init_i(uc_int *x, int n)
{
    uc_init(x);
    return uc_set_i(x, n);
}

/*
 * Initialize UC integer x with built-in long n.
 */
int uc_init_l(uc_int *x, long n)
{
    uc_init(x);
    return uc_set_l(x, n);
}

/*
 * Initialize UC integer x with digit n.
 */
int uc_init_d(uc_int *x, uc_digit n)
{
    uc_init(x);
    uc_grow(x, sizeof(n) / sizeof(uc_digit) + 2);

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
        d += (n & 1) << (i % UC_DIGIT_BITS);
        if ((i + 1) % UC_DIGIT_BITS == 0 )
        {
            x->digits[digit_ctr++] = d;
            x->used++;
            d = 0;
        }
    }

    // Add final digit
    x->digits[digit_ctr] = d;
    x->used++;

    uc_clamp(x);

    return UC_OK;
}

int uc_init_w(uc_int *x, uc_word n)
{
    uc_init(x);
    return uc_set_w(x, n);
}

/*
 * For any non-zero number, make sure that x->digits[x->used-1] is a non-zero digit. If
 * x is zero, we set x->used to 1.
 *
 * For example, the clamped version of
 * x = [ b_0 b_1 b_2 ... b_k 0 0 0 0 0 0 0 ]
 *                             ^
 *                             |
 *                             x_used
 * is (assuming b_k is not zero)
 * x = [ b_0 b_1 b_2 ... b_3 0 0 0 0 0 0 0 ]
 *                        ^
 *                        |
 *                        x_used
 *
 * If x is zero, the clamped version looks as follows:
 * x = [ 0 0 0 0 ... 0 0 0 0 0 0 0 0 ]
 *       ^
 *       |
 *       x_used
 */
int uc_clamp(uc_int *x)
{
    while ( x->used > 1 && x->digits[x->used - 1] == 0 )
        --(x->used);

    return UC_OK;
}


/*
 * Assign zero to x (i.e. x := 0)
 */
int uc_set_zero(uc_int *x)
{
    if ( x->digits == NULL )
        uc_grow(x, 1);

    for ( int i = 0; i < x->used; ++i )
        x->digits[i] = 0;
    x->used = 1;
    x->sign = UC_POS;

    return UC_OK;
}

/*
 * Assign n to x
 */
int uc_set_i(uc_int *x, int n)
{
    uc_set_l(x, n);
}

/*
 * Assign n to x
 */
int uc_set_l(uc_int *x, long n)
{
    uc_set_zero(x);
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
        d += (n & 1) << (i % UC_DIGIT_BITS);
        if ((i + 1) % UC_DIGIT_BITS == 0 )
        {
            x->digits[digit_ctr++] = d;
            x->used++;
            d = 0;
        }
    }

    // Add final digit
    x->digits[digit_ctr] = d;
    x->used++;

    uc_clamp(x);

    return UC_OK;
}

int uc_set_d(uc_int *x, uc_digit n)
{
    uc_set_w(x, n);
}

int uc_set_w(uc_int *x, uc_word n)
{
    uc_set_zero(x);
    uc_grow(x, sizeof(n) / sizeof(uc_word) + 2);

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
        d += (n & 1) << (i % UC_DIGIT_BITS);
        if ((i + 1) % UC_DIGIT_BITS == 0 )
        {
            x->digits[digit_ctr++] = d;
            x->used++;
            d = 0;
        }
    }

    // Add final digit
    x->digits[digit_ctr] = d;
    x->used++;

    uc_clamp(x);

    return UC_OK;
}

/*
 * Copy y to x (i.e. x := y)
 */
int uc_copy(uc_int *x, uc_int *y)
{
    int i;

    if ( y->used > x->used )
        uc_grow(x, y->used);

    /* Copy y to x digit by digit */
    for ( i = 0; i < y->used; ++i )
        x->digits[i] = y->digits[i];

    /* Zero out the remaining digits of x */
    for ( ; i < x->used; ++i )
        x->digits[i] = 0;

    x->used = y->used;
    x->sign = y->sign;

    return UC_OK;
}

/*
 * Release memory allocated for UC integer x. For security reasons, we zero out
 * the memory.
 */
int uc_free(uc_int *x)
{
    uc_set_zero(x);
    XFREE(x->digits);
    x->digits = NULL;
    x->used = 0;
    x->alloc = 0;
    x->sign = UC_POS;

    return UC_OK;
}

int uc_free_multi(uc_int *x0, uc_int *x1, uc_int *x2, uc_int *x3, uc_int *x4, uc_int *x5)
{
    if ( x0 ) uc_free(x0);
    if ( x1 ) uc_free(x1);
    if ( x2 ) uc_free(x2);
    if ( x3 ) uc_free(x3);
    if ( x4 ) uc_free(x4);
    if ( x5 ) uc_free(x5);
}

/*
 * Comparisons
 */

/* Compare two integers (signed)
 * x < y  ==> returns UC_LT
 * x == y ==> returns UC_EQ
 * x > y ==> returns UC_GT
 * */
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

/*
 * Compute z = x + y
 */
int uc_add(uc_int *z, uc_int *x, uc_int *y)
{
    /*
     * Ensure that |x| >= |y| (this is required by the base addition algorithm).
     *
     * If |x| >= |y|, there is nothing to do
     * If |x| < |y|,  we switch x and y
     *
     * This works since x + y = y + x
     */
    if ( uc_cmp_mag(x, y) == UC_LT )
    {
        uc_int *swap = x;
        x = y;
        y = swap;
    }

    assert( uc_cmp_mag(x, y) != UC_LT); // Henceforth, x >= y

    int xs = x->sign;
    int ys = y->sign;

    int status;

    if ( xs == UC_POS && ys == UC_POS )
    {
        status = _uc_add(z, x, y);
        z->sign = UC_POS;
    }
    else if ( xs == UC_NEG && ys == UC_POS )
    {
        status = _uc_sub(z, x, y);
        z->sign = UC_NEG;
    }
    else if ( xs == UC_POS && ys == UC_NEG )
    {
        status = _uc_sub(z, x, y);
        z->sign = UC_POS;
    }
    else // xs == UC_NEG && ys == UC_NEG
    {
        status = _uc_add(z, x, y);
        z->sign = UC_NEG;
    }

    return status;
}

/*
 * Compute z = x + y for x >= y >= 0.
 */
static int _uc_add(uc_int *z, uc_int *x, uc_int *y)
{
    assert(uc_cmp_mag(x, y) != UC_LT);

    int i, res;
    uc_digit c, tmp;

    res = UC_OK;

    /* Ensure that z is initialized with 0 and that there is enough space to hold the result */
    if ( (res = uc_grow(z, x->used + 1)) != UC_OK )
        return res;

    c = 0;     /* carry */

    for ( i = 0; i < y->used; ++i )
    {
        tmp = x->digits[i] + y->digits[i] + c;
        c = tmp >> UC_DIGIT_BITS;
        z->digits[i] = tmp & UC_DIGIT_MASK;
    }

    for ( ; i < x->used; ++i )
    {
        tmp = x->digits[i] + c;
        c = tmp >> UC_DIGIT_BITS;
        z->digits[i] = tmp & UC_DIGIT_MASK;
    }

    if (c > 0 )
        z->digits[i++] = c;

    for ( ; i < z->alloc; ++i )
        z->digits[i] = 0;

    z->used = x->used + 1;
    res = uc_clamp(z);

    return res;
}

/*
 * Compute z = x + y where x,y UC integers and d digit.
 */
int uc_add_d(uc_int *z, uc_int *x, uc_digit d)
{
    uc_int y;
    uc_init_d(&y, d);
    int status = uc_add(z, x, &y);
    uc_free(&y);
    return status;
}

/*
 * Calculate z = x - y for UC integers x and y
 */
int uc_sub(uc_int *z, uc_int *x, uc_int *y)
{
    /*
     * Ensure that |x| >= |y| (this is required by the base subtraction algorithm).
     *
     * If |x| >= |y|, there is nothing to do
     * If |x| < |y|,  we swich x and y and remeber to flip the sign in the end since -(x - y) = (y - x)
     */
    int flip_sign = 0;
    if ( uc_cmp_mag(x, y) == UC_LT )
    {
        uc_int *swap = x;
        x = y;
        y = swap;
        flip_sign = 1;
    }

    assert( uc_cmp_mag(x, y) != UC_LT );

    int xs = x->sign;
    int ys = y->sign;

    /* There are four combinations for (xs,ys) which we need to handle separately */

    int status;

    if ( xs == UC_POS  && ys == UC_POS )
    {
        status = _uc_sub(z, x, y);
        z->sign = UC_POS;
    }
    else if ( xs == UC_NEG && ys == UC_POS )
    {
        status = _uc_add(z, x, y);
        z->sign = UC_NEG;
    }
    else if ( xs == UC_POS && ys == UC_NEG )
    {
        status = _uc_add(z, x, y);
        z->sign = UC_POS;
    }
    else // xs == UC_NEG && ys == UC_NEG
    {
        status = _uc_sub(z, x, y);
        z->sign = UC_NEG;
    }

    /* Flip sign if x and y were swapped */
    if ( flip_sign )
        uc_flip_sign(z);

    return status;
}

/*
 * Compute z = x - y with |x| >= |y| and x, y >= 0
 */
static int _uc_sub(uc_int *z, uc_int *x, uc_int *y)
{
    assert(uc_cmp_mag(x, y) != UC_LT);

    int res;

    res = UC_OK;

    /* Ensure that z can hold result */
    if ( (res = uc_grow(z, x->used))  != UC_OK )
        return res;

    int i;
    uc_digit c = 0; // carry
    uc_digit tmp;
    for ( i = 0; i < y->used; ++i )
    {
        uc_digit x_i = x->digits[i];
        uc_digit y_i = y->digits[i];

        tmp = x_i - y_i - c;

        c = tmp >> ((uc_digit)(8 * sizeof (uc_digit) - 1));

        z->digits[i] = tmp & UC_DIGIT_MASK;
    }

    for ( ; i < x->used; ++i )
    {
        tmp = x->digits[i] - c;
        c = tmp >> ((uc_digit)(8 * sizeof (uc_digit) - 1));
        z->digits[i] = tmp & UC_DIGIT_MASK;
    }

    for ( ; i < z->alloc; ++i )
        z->digits[i] = 0;

    z->used = x->used;
    res = uc_clamp(z);

    return res;
}

/*
 * Compute z = x * y
 */
int uc_mul(uc_int *z, uc_int *x, uc_int *y)
{
    int res;

    res = _uc_mul(z, x, y);
    if ( x->sign != y->sign )
        z->sign = UC_NEG;

    return res;
}

/*
 * Compute z = |x| * |y|
 */
static int _uc_mul(uc_int *z, uc_int *x, uc_int *y)
{
    int n = x->used;
    int m = y->used;

    /* Allocate enough space, zero out memory, and set minimum number of digits */
    uc_set_zero(z);
    uc_grow(z, n + m + 2);
    z->used = n + m + 2;  // Let's be generous - we can clamp at the end

    for ( int i = 0; i < m; ++i )
    {
        uc_word c  = 0;     // carry
        for ( int j = 0; j < n; ++j )
        {
            // tmp = r_ij + x_j * y_i + c
            uc_word tmp = ((uc_word) z->digits[i + j]) +
                          ((uc_word) x->digits[j]) *
                          ((uc_word) y->digits[i]) +
                          ((uc_word) c);

            z->digits[i + j] = tmp & UC_DIGIT_MASK;
            c = tmp >> UC_DIGIT_BITS;
        }

        z->digits[i + n] = c;
    }

    uc_clamp(z);

    return UC_OK;
}

/*
 * Compute z = x * y for z, x UC integers and UC digit y.
 */
int uc_mul_d(uc_int *z, uc_int *x, uc_digit y)
{
    int i, res;
    uc_word u, r;

    res = UC_OK;

    /*
     * A few special cases
     */
    if (uc_is_zero(x) || y == 0 )
        return uc_set_zero(z);
    if (y == 0 )
        return uc_copy(z, x);
    if ( uc_is_one(x) )
        return uc_set_i(z, y);

    /* Ensure that z can hold result */
    if ( (res = uc_grow(z, x->used + 1)) != UC_OK )
        return res;

    u = 0;
    for ( i = 0; i < x->used; ++i )
    {
        r = u + ((uc_word) x->digits[i]) * y;
        z->digits[i] = r % UC_INT_BASE;
        u = r / UC_INT_BASE;
    }
    z->digits[i++] = u;

    for ( ; i < z->used; ++i )
        z->digits[i] = 0;

    z->sign = x->sign;

    if ( z->used < x->used + 1 )
        z->used = x->used + 1;
    return uc_clamp(z);
}

/*
 * Compute x = ys[0] * ys[1] * ... * ys[k-1]
 */
int uc_mul_multi(uc_int *x, uc_int *ys, int k)
{
    int i, res;
    uc_int tmp;

    if ( k < 1 )
        return UC_INPUT_ERR;

    res = UC_OK;
    if ( (res = uc_init(&tmp)) != UC_OK )
        return res;

    /* x = 1 */
    if ( (res = uc_set_i(x, 1)) != UC_OK )
        goto cleanup;

    for ( i = 0; i < k; ++i )
    {
        /* x = x * ys[i] */
        if ( (res = uc_mul(&tmp, x, ys + i)) != UC_OK ||
             (res = uc_copy(x, &tmp)) != UC_OK )
        {
            goto cleanup;
        }
    }

cleanup:
    uc_free(&tmp);

    return res;

}

/*
 * Computes x = y^2
 */
int uc_sqrd(uc_int *x, uc_int *y)
{
    return uc_mul(x, y, y); // TODO: Implement more efficient algorithm
}

/*
 * Compute x = 2 * y
 *
 * TODO: Use a more efficient algorithm
 */
int uc_mul_2(uc_int *x, uc_int *y)
{
    uc_int tmp;
    int status = uc_init_i(&tmp, 2);
    uc_mul(x, y, &tmp);
    uc_free(&tmp);
    return status;
}

// TODO: add to header file
int uc_exch (uc_int * a, uc_int * b)
{
    uc_int  t;

    t  = *a;
    *a = *b;
    *b = t;
    return UC_OK;
}

static int _check_div(uc_int *q, uc_int *r, uc_int *x, uc_int *y)
{
    uc_int t1, t2;

    uc_init(&t1);
    uc_init(&t2);

    uc_mul(&t1, q, y);
    uc_add(&t2, &t1, r);

    if ( !uc_eq_mag(x, &t2) )
    {
        puts("[!] _check_div FAILED. Exiting know...");
        exit(1);
    }

    uc_free(&t1);
    uc_free(&t2);
}

/*
 * Compute r and q s.t. x = q * y + r for non-negative integers x and y.
 */
int uc_div(uc_int *q, uc_int *r, uc_int *x, uc_int *y)
{
    int k;
    uc_int xt, yt, tmp;

    /*
     * Bad things happen if we divide by zero.
     */
    if ( uc_is_zero(y) )
        return UC_INPUT_ERR;

    /*
     * If x < y, we know that q = 0 and r = x
     */
    if ( uc_lt(x, y) )
    {
        uc_set_zero(q);
        uc_copy(r, x);
        return UC_OK;
    }

    /*
     * If x == y, we know that q = 1 and r = 0;
     */
    if ( uc_eq(x, y) )
    {
        uc_init_i(q, 1);
        uc_set_zero(r);
        return UC_OK;
    }

    uc_init_multi(&xt, &yt, &tmp, 0, 0, 0);

    uc_copy(&xt, x);        // x'
    uc_copy(&yt, y);        // y'

    /*
     * Normalize y' (i.e. ensure that the most significant digit of y is at least BASE // 2.
     *
     * More formally, find smallest k s.t. most significant digit of y' is greater or equal to BASE // 2
     * where
     * x' = 2^k * x
     * y' = 2*k * y
     *
     * When then calculate with x' and y' and normalize r and q in the end.
     */
    for ( k = 0; yt.digits[yt.used-1] < UC_INT_BASE / ((uc_word)2); ++k )
    {
        uc_lshb(&tmp, &xt, 1);
        uc_copy(&xt, &tmp);

        uc_lshb(&tmp, &yt, 1);
        uc_copy(&yt, &tmp);
    }

    _uc_div(q, r, &xt, &yt);

    /*
     * Divide r by 2^k to compensate for the normalization.
     *
     * There is no need to change q as it is not affected by the normalization since
     * x' // y' == (x * 2^k) // (y * 2^k) = x // y
     */
    uc_rshb(&tmp, r, k);
    uc_copy(r, &tmp);

    /* Cleanup local variables */
    uc_free(&xt);
    uc_free(&yt);
    uc_free(&tmp);

    return UC_OK;
}

/*
 * Compute r and q s.t. x = q * y + r where r < y. The following preconditions must be met:
 * - y is normalized (i.e., its most significant digit is at least BASE / 2)
 * - y contains at least two digits.
 */
// TODO: implement case where y->used = 1
static int _uc_div(uc_int *q, uc_int *r, uc_int *x, uc_int *y)
{
    int m, n;
    int i, j;
    uc_word q_estimate;
    uc_int ta, tb, tc;

    uc_init(&ta);
    uc_init(&tb);
    uc_init(&tc);

    n = y->used;
    m = x->used - n;

    assert( y->digits[n-1] >= UC_INT_BASE / ((uc_word) 2));

    uc_grow(q, m + 1); // TODO: this can maybe be smaller?
    uc_set_zero(q);
    q->used = q->alloc; // TODO: is this necessary?

    /*
     * Step 1
     */
    /* ta = base^m * y */
    uc_copy(&ta, y);
    uc_lshd(&ta, m);

    if ( uc_gte(x, &ta) )
    {
        q->digits[m] = 1;
        uc_sub(&tb, x, &ta);
        uc_copy(x, &tb); // TODO: this copy is not needed (we don't need tb)
    }

    /*
     * Steps 2 - 8
     */
    for ( j = m - 1; j >= 0; --j )
    {
        /*
         * Quotient estimation
         */
        // TODO: check that x is large enough
        q_estimate = ( (((uc_word) x->digits[n+j]) * UC_INT_BASE) + ((uc_word) x->digits[n+j-1]) ) / ((uc_word) y->digits[n-1]);
        if ( q_estimate > (UC_INT_BASE - 1) )
            q_estimate = UC_INT_BASE - 1;
        q->digits[j] = q_estimate;

        /*
         * x = x - q_estimate * (base ^ j) * y
         */
        uc_mul_d(&ta, y, q_estimate);   // ta = y * q_estimate
        uc_lshd(&ta, j);                // ta = ta * (base ^ j)
        uc_sub(&tb, x, &ta);
        uc_copy(x, &tb);

        uc_set_zero(&ta);
        while ( uc_lt(x, &ta) )
        {
            q->digits[j]--;
            uc_copy(&tb, y);
            uc_lshd(&tb, j);
            uc_add(&tc, x, &tb);
            uc_copy(x, &tc);
        }
    }

    uc_copy(r, x);

    uc_clamp(r);
    uc_clamp(q);

    uc_free(&ta);
    uc_free(&tb);
    uc_free(&tc);

    return UC_OK;
}

/*
 * Compute x = y // 2, i.e.:
 *   y is even ==> x = y / 2
 *   y odd ==> x = (y - 1) / 2
 */
int uc_div_2(uc_int *x, uc_int *y)
{
    /* Division by two is equivalent to a right shift by one bit */
    return uc_rshb(x, y, 1);
}

/*
 * Compute UC integer q and UC digit r s.t. x = q * y + r
 */
int uc_div_d(uc_int *q, uc_digit *r, uc_int *x, uc_digit y)
{
    int i, res;
    uc_word w, t;

    res = UC_OK;

    if ( y == 0 )
        return UC_INPUT_ERR;

    if ( uc_is_zero(x) )
    {
        uc_set_zero(q);
        *r = 0;
    }

    if ( (res = uc_grow(q, x->used)) != UC_OK )
        return res;

    w = 0;
    for ( i = x->used - 1; i >= 0; --i )
    {
        w = (w << ((uc_word) UC_DIGIT_BITS)) + x->digits[i];
        if ( w >= y )
        {
            t = w / y;
            w %= y;
        }
        else
            t = 0;

        q->digits[i] = t;
    }

    *r = w;

    q->used = x->used;
    uc_clamp(q);

    return res;
}

/*
 * Compute z = x ^ y for y >= 0.
 */
int uc_exp(uc_int *z, uc_int *x, uc_int *y)
{
    assert( !uc_is_neg(y) );

    uc_int tmp;
    int i, n, res;

    res = UC_OK;

    /*
     * If y is zero, simply set z to 1.
     *
     * Note: We use the convention that 0^0 = 1.
     */
    if ( uc_is_zero(y) )
        return uc_set_i(z, 1);

    if ( (res = uc_init(&tmp)) != UC_OK ||
         (res = uc_set_i(z, 1)) != UC_OK )
    {
        goto cleanup;
    }

    /*
     * Square and multiply. (Note: we ignore the sign during this loop and
     * fix it later).
     */
    n = uc_count_bits(y);
    for ( i = n - 1; i >= 0; --i )
    {
        /* z = z * z */
        if ( (res = uc_sqrd(&tmp, z)) != UC_OK ||
             (res = uc_copy(z, &tmp)) != UC_OK )
        {
            goto cleanup;
        }

        /*
         * z = z * x ^ (y_i) where y_i is i-th exponent bit.
         *
         * To avoid time-based side channel information leakage, we carry out the
         * multiplication and result copy operation regardless of the exponent
         * bit y_i.
         */

        if ( (res = uc_mul(&tmp, z, x)) != UC_OK )
            goto cleanup;

        if ( uc_nth_bit(y,i) == 1 )
            uc_copy(z, &tmp);
        else
            uc_copy(z, z);
    }

    /* Fix sign */
    if ( uc_is_neg(x) && uc_is_odd(y) )
        z->sign = UC_NEG;
    else
        z->sign = UC_POS;

cleanup:
    uc_free(&tmp);

    return res;
}

/*
 * Compute z = x ^ y
 */
int uc_exp_i(uc_int *z, uc_int *x, int y)
{
    int res;
    uc_int yt;

    res = UC_OK;

    if ( (res = uc_init(&yt)) != UC_OK )
        return res;

    if ( (res = uc_set_i(&yt, y)) != UC_OK ||
         (res = uc_exp(z, x, &yt)) != UC_OK )
    {
        goto cleanup;
    }

cleanup:
    uc_free(&yt);

    return res;
}

/*
 * Compute x = (y << n) for n >= 0
 *
 * I.e., shift y left by n bits and store result in x
 */
int uc_lshb(uc_int *x, uc_int *y, int n)
{
    assert(n >= 0);

    /* Nothing to do */
    if ( n == 0 )
        return uc_copy(x, y);

    /*
     * Nothing to do if x is zero.
     *
     * Warning: Do not remove this check unless you clamp the number at the very end (otherwise
     * you end up with a non-normalized zero).
    */
    if ( uc_is_zero(y) )
        return uc_copy(x, y);

    /* Initialize x */
    uc_copy(x, y);

    /* Ensure that x can hold result */
    if ( x->alloc < x->used + (n / UC_DIGIT_BITS) + 1)
        uc_grow(x, x->used + (n / UC_DIGIT_BITS) + 1);

    /*
     *  If n = a * DIGITS + b, we can shift by _a_ digits and then by _b_ bits, which simplifies
     *  the algorithm.
     */
    if (n >= UC_DIGIT_BITS )
    {
        uc_lshd(x, n / UC_DIGIT_BITS);
        n %= UC_DIGIT_BITS;
    }

    /* If b = 0 we are already done */
    if ( n == 0 )
        return UC_OK;

    for ( int i = x->used - 1; i >= 0; --i )
    {
        uc_digit shift = UC_DIGIT_BITS - n;

        uc_digit mask = (((uc_digit)1u) << n) - 1;

        uc_digit lsb = (x->digits[i] << n) & UC_DIGIT_MASK;
        uc_digit msb = (x->digits[i] >> shift) & mask;
        x->digits[i] = lsb;
        x->digits[i+1] |= msb;
    }

    x->used = x->alloc;
    uc_clamp(x);

    return UC_OK;
}

/*
 * Compute x = (y >> n) for n >= 0
 *
 * I.e., shift y right by n bits and store result in x
 */
int uc_rshb(uc_int *x, uc_int *y, int n)
{
    assert( n >= 0 );

    if ( n == 0 )
        return uc_copy(x, y);

    /*
     * Nothing to do if x is zero.
     *
     * Warning: Do not remove this check unless you clamp the number at the very end (otherwise
     * you end up with a non-normalized zero).
     */
    if ( uc_is_zero(y) )
        return uc_copy(x, y);

    /* Initialize x */
    uc_copy(x, y);

    /*
     *  Note: After copying y to x, we know that x can hold the final result since
     *  x <= (y >> n). Therefore, we need not check if we need to grow x.
     */

    /*
     *  If n = a * DIGITS + b, we can shift by _a_ digits and then by _b_ bits, which simplifies
     *  the algorithm.
     */
    if (n >= UC_DIGIT_BITS )
    {
        uc_rshd(x, n / UC_DIGIT_BITS);
        n %= UC_DIGIT_BITS;
    }

    /* If b = 0 we are already done */
    if ( n == 0 )
        return UC_OK;

    uc_digit mask = (((uc_digit) 1) << n) - ((uc_digit) 1);
    uc_digit r = 0;
    for ( int i = x->used - 1; i >= 0; --i )
    {
        uc_digit rr = x->digits[i] & mask;
        x->digits[i] = (x->digits[i] >> n) | (r << (UC_DIGIT_BITS - n));
        r = rr;
    }

    uc_clamp(x);
}

/*
 * Shift x left by y >= digits.
 *
 * For example, if y = 3:
 * [x_0 x_1 x_2 x_3 ... x_{n-1}] --> [0 0 0 x_0 x_1 x_2 x_3 ... x_{n-1}]
 * (Note: The array above depicts the actual internal representation in x->digits)
 */
int uc_lshd(uc_int *x, int y)
{
    assert(y >= 0);

    /* Nothing to do */
    if ( y == 0 )
        return UC_OK;

    /*
     * Nothing to do if x is zero.
     *
     * Warning: Do not remove this check unless you clamp the number at the very end (otherwise
     * you end up with a non-normalized zero).
     */
    if ( uc_is_zero(x) )
        return UC_OK;

    uc_grow(x, x->used + y);

    /* Iterate backwards (i.e. from x_{n-1} to x_0) and copy x_i to x_{i+y} */
    for ( int i = x->used - 1; i >= 0; --i )
        x->digits[i + y] = x->digits[i];

    /* Zero out the first y digits */
    for ( int i = 0; i < y; ++i )
        x->digits[i] = 0;

    x->used += y;
}

/*
 * Shift x right by y >= 0 digits.
 *
 * For example, if y = 3:
 * [x_0 x_1 x_2 x-3 ... x_n] --> [x_3 x_4 ... x_n 0 0 0]
 * (Note: The array above depicts the actual internal representation in x->digits)
 */
int uc_rshd(uc_int *x, int y)
{
    assert(y >= 0);

    /* Nothing to do */
    if ( y == 0 )
        return UC_OK;

    /*
     * Nothing to do if x is zero.
     *
     * Warning: Do not remove this check unless you clamp the number at the very end (otherwise
     * you end up with a non-normalized zero).
     */
    if ( uc_is_zero(x) )
        return UC_OK;

    int i;

    for ( i = 0; i < x->used - y; ++i )
        x->digits[i] = x->digits[i+y];

    for ( ; i < x->used; ++i )
        x->digits[i] = 0;

    x->used -= y;
}

/*
 * x = |y|
 */
int uc_abs(uc_int *x, uc_int *y)
{
    int status = uc_copy(x, y);
    x->sign = UC_POS;
    return status;
}

int uc_flip_sign(uc_int *x)
{
    if ( uc_is_zero(x) ) // do nothing for zero (it's sign is always positive)
        return UC_OK;

    if ( x->sign == UC_POS )
        x->sign = UC_NEG;
    else
        x->sign = UC_POS;

    return UC_OK;
}

/*
 * Count the number of bits in x.
 *
 */
int uc_count_bits(uc_int *x)
{
    assert( x->digits != NULL );

    /*
     * TODO:
     *
     * Open question: How many bits are required for zero?
     */
    if (uc_is_zero(x) )
        return 1;

    int nbits = (x->used - 1) * UC_DIGIT_BITS;

    /*
     * Let the most significant digit be [d_0, d_1, ..., d_{UC_DIGIT_BITS-1}]. We look for the largest i
     * (in descending order) s.t. d_i != 0 and then add (i+1) to the current count.
     */
    for (int i = UC_DIGIT_BITS - 1; i >= 0; --i )
    {
        if ( x->digits[x->used-1] & (1u << i) )
        {
            nbits += (i + 1);
            break;
        }
    }

    return nbits;
}

/*
 * Modular artithmetic
 */

/*
 * Compute z = (x + y) (mod m) for 0 <= x,y < m
 */
int uc_add_mod(uc_int *z, uc_int *x, uc_int *y, uc_int *m)
{
    uc_int tmp;

    uc_init(&tmp);

    /* check that 0 <= x < m */
    if (uc_is_neg(x) || !uc_lt(x, m) )
        return UC_INPUT_ERR;

    /* check that 0 <= y < m */
    if (uc_is_neg(y) || !uc_lt(y, m) )
        return UC_INPUT_ERR;

    /* At this point we implicitly know that m > 0 */

    uc_add(z, x, y);
    if ( uc_gte(z, m) )
    {
        uc_sub(&tmp, z, m);
        uc_copy(z, &tmp);
    }

    uc_free(&tmp);

    return UC_OK;
}

/*
 * Compute z = x * y (mod m) for 0 <= x, y < m and m >= 0
 */
int uc_mul_mod(uc_int *z, uc_int *x, uc_int *y, uc_int *m)
{
    int res;
    uc_int tmp;

    res = UC_OK;

    if ( (res = uc_init(&tmp)) != UC_OK )
        return res;

    if ( (res = uc_mul(&tmp, x, y)) != UC_OK ||
         (res = uc_mod(z, &tmp, m)) != UC_OK )
    {
        goto cleanup;
    }

cleanup:
    uc_free(&tmp);

    return res;
}

/*
 * Compute inverse x of y modulo m, i.e., s.t. x * y = 1 (mod m)
 *
 * Note: In Modern Computer Arithmetic, different variables names are used:
 * x ==> u
 * y ==> b
 * m ==> N
 */
int uc_mod_inv(uc_int *x, uc_int *y, uc_int *m)
{
    int res;
    uc_int c, w, q, r, yt, tmp;

    res = UC_OK;

    if ((res = uc_init_multi(&c, &tmp, &w, &q, &r, &yt)) != UC_OK )
        return res;

    if ((res = uc_copy(&yt, y)) != UC_OK )
        goto cleanup;

    if ((res = uc_set_i(x, 1)) != UC_OK ||
        (res = uc_set_zero(&w)) != UC_OK ||
        (res = uc_copy(&c, m)) != UC_OK )
    {
        goto cleanup;
    }

    while ( !uc_is_zero(&c) )
    {
        if ((res = uc_div(&q, &r, &yt, &c)) != UC_OK )
            goto cleanup;

        /*
         * (y, c) := (c, r)
         */

        if ((res = uc_copy(&yt, &c)) != UC_OK ||
             (res = uc_copy(&c, &r)) != UC_OK )
        {
            goto cleanup;
        }

        /*
         * (x, w) = (w, x - q * w)
         */

        /* use r as temporary variable for w (it's not needed anymore until the next loop iteration */
        if ( (res = uc_copy(&r, &w)) != UC_OK)
            goto cleanup;

        if ((res = uc_mul_mod(&tmp, &q, &w, m)) != UC_OK ||
             (res = uc_sub(&w, x, &tmp)) != UC_OK )
        {
            goto cleanup;
        }

        /* If w < 0, add m to make it non-negative again */
        if (uc_is_neg(&w) )
        {
            uc_add(&tmp, &w, m);
            uc_copy(&w, &tmp);
        }
        assert( !uc_is_neg(&w) );

        uc_copy(x, &r);
    }

cleanup:
    uc_free_multi(&c, &tmp, &w, &q, &r, &yt);

    return res;
}

/*
 * Compute x = y (mod m) for y >= 0 and m > 0
 */
int uc_mod(uc_int *x, uc_int *y, uc_int *m)
{
    int res;
    uc_int qt;

    res = UC_OK;

    /* Check that y >= 0 and m > 0 */
    if ( uc_is_neg(y) || !uc_is_pos(m) )
    {
        return UC_INPUT_ERR;
    }

    /* No reductions needed if y < m */
    if ( uc_lt(y, m) )
        return uc_copy(x, y);

    if ((res = uc_init(&qt)) != UC_OK ||
        (res = uc_div(&qt, x, y, m)) != UC_OK )
    {
        goto cleanup;
    }

cleanup:
    uc_free(&qt);

    return res;
}

int uc_gcd(uc_int *z, uc_int *u, uc_int *v)
{
    int res;
    uc_int vt, tmp, tmp2;

    if ( !uc_is_pos(u) || !uc_is_pos(v) )
        return UC_INPUT_ERR;

    res = UC_OK;

    if ( (res = uc_init_multi(&vt, &tmp, &tmp2, 0, 0, 0)) != UC_OK )
        return res;

    if  ( (res = uc_copy(z, u)) != UC_OK ||
          (res = uc_copy(&vt, v)) != UC_OK )
    {
        goto leave;
    }

    while ( !uc_is_zero(&vt) )
    {
        if ( (res = uc_copy(&tmp, z)) != UC_OK ||
             (res = uc_copy(z, &vt)) != UC_OK ||
             (res = uc_mod(&tmp2, &tmp, &vt)) != UC_OK ||
             (res = uc_copy(&vt, &tmp2)) != UC_OK  )
        {
            goto leave;
        }
    }

leave:
    uc_free(&vt);
    uc_free(&tmp);
    uc_free(&tmp2);

    return res;
}

/*
 * Calculate GCD for two positive integers (uc_word) x and y.
 */
uc_word uc_gcd_word(uc_word x, uc_word y)
{
    return x >= y ? _uc_gcd_word(x, y) : _uc_gcd_word(y, x);
}

int uc_egcd(uc_int *g, uc_int *u, uc_int *v, uc_int *a, uc_int *b)
{
    int res;
    uc_int w, x, q, r, bt, tmp;

    if ( (res = uc_init_multi(&w, &x, &q, &r, &bt, &tmp)) != UC_OK )
        return res;

    if ( (res = uc_copy(g, a) != UC_OK) ||
         (res = uc_copy(&bt, b)) != UC_OK )
    {
        goto cleanup;
    }

    /* (u, w) <-- (1, 0) */
    if ( (res = uc_set_i(u, 1)) != UC_OK ||
         (res = uc_set_zero(&w)) != UC_OK )
    {
        goto cleanup;
    }

    /* (v, x) <-- (0, 1) */
    if ( (res = uc_set_zero(v)) != UC_OK ||
         (res = uc_set_i(&x, 1)) != UC_OK )
    {
        goto cleanup;
    }

    while ( !uc_is_zero(&bt) )
    {
        if ( (res = uc_div(&q, &r, g, &bt)) != UC_OK )
            goto cleanup;

        /* (a, b) <-- (b, r) */
        if ( (res = uc_copy(g, &bt)) != UC_OK ||
             (res = uc_copy(&bt, &r)) != UC_OK )
        {
            goto cleanup;
        }

        /*
         * (u, w) <-- (w, u - q * w)
         */
        if ( (res = uc_copy(&r, &w)) != UC_OK ||
             (res = uc_mul(&tmp, &q, &w)) != UC_OK ||
             (res = uc_sub(&w, u, &tmp)) != UC_OK ||
             (res = uc_copy(u, &r)) != UC_OK )
        {
            goto cleanup;
        }

        /*
         * (v, x) <-- (x, v - q * x)
         */
        if ( (res = uc_copy(&r, &x)) != UC_OK ||
                (res = uc_mul(&tmp, &q, &x)) != UC_OK ||
                (res = uc_sub(&x, v, &tmp)) != UC_OK ||
                (res = uc_copy(v, &r)) != UC_OK )
        {
            goto cleanup;
        }
    }

cleanup:
    uc_free_multi(&w, &x, &q, &r, &bt, &tmp);

    return res;
}

/*
 * Calculate GCD for two positive integers (uc_word) with x >= y
 */
static uc_word _uc_gcd_word(uc_word x, uc_word y)
{
    uc_word g = 1;
    while ( x % 2 == 0 && y % 2 == 0 )
    {
        x /= 2;
        y /= 2;
        g *= 2;
    }

    while ( x != 0 )
    {
        while ( x % 2 == 0 )
            x /= 2;
        while ( y % 2 == 0 )
            y /= 2;

        uc_word tmp = (x >= y ? (x - y) : (y - x)) / 2;     // |x - y| / 2
        if ( x >= y )
            x = tmp;
        else
            y = tmp;
    }

    return g * y;
}

/*
 * Converts integer to residue number system.
 *
 * Formally:
 * For integer x and moduli ms[0], ms[1], ..., ms[k-1], compute
 *     x[i] = x mod ms[i]
 * for 0 <= i < k.
 */
int uc_int2rns(uc_int *xs, uc_int *x, uc_int *ms, int k)
{
    int res, l;
    uc_int M, tmp;

    /* Base Case 1 */
    if ( k == 1 )
        return uc_mod(xs, x, ms);

    /* Base Case 2 */
    if ( k == 2 )
    {
        if ( (res = uc_mod(xs, x, ms)) != UC_OK )
            return res;
        return uc_mod(xs + 1, x, ms + 1);
    }

    if ( (res = uc_init_multi(&M, &tmp, 0, 0, 0, 0)) != UC_OK )
        return res;


    l = k / 2;

    /* "Left" side of recursion */
    if ( (res = uc_mul_multi(&M, ms, l)) != UC_OK ||
         (res = uc_mod(&tmp, x, &M)) != UC_OK ||
         (res = uc_int2rns(xs, &tmp, ms, l)) != UC_OK )
    {
        goto cleanup;
    }

    /* "Right" side of recursion */
    if ( (res = uc_mul_multi(&M, ms + l, k - l)) != UC_OK ||
         (res = uc_mod(&tmp, x, &M)) != UC_OK ||
         (res = uc_int2rns(xs + l, x, ms + l, k - l)) != UC_OK )
    {
        goto cleanup;
    }

cleanup:
    uc_free(&M);
    uc_free(&tmp);

    return res;
}

/*
 * Chinese Remainder Theorem. Formally:
 *
 * Compute x s.t.
 *   0 <= x < ms[0] * ms[1] * ... * ms[k-1]
 * and
 *  x = xs[i] mod ms[i] for all 0 <= i < k.
 */
int uc_rns2int(uc_int *x, uc_int *xs, uc_int *ms, int k)
{
    int res, l;
    uc_int m1, m2, x1, x2;
    uc_int lambda1, lambda2;
    uc_int tmp, u, v;

    /* Recursion base case */
    if ( k == 1 )
        return uc_copy(x, xs);

    /* Initialize local variables */
    if ( (res = uc_init_multi(&m1, &m2, &x1, &x2, &lambda1, &lambda2)) != UC_OK )
        return res;
    if ( (res = uc_init_multi(&tmp, &u, &v, 0, 0, 0)) != UC_OK )
        goto cleanup2;

    /* Recursion */

    l = k / 2;

    uc_rns2int(&x1, xs, ms, l);
    uc_rns2int(&x2, xs + l, ms + l, k - l);

    /* Combine result */

    /*
     * Compute u, v s.t. u * m1 + v * m2 = 1
     */
    uc_mul_multi(&m1, ms, l);
    uc_mul_multi(&m2, ms + l, k - l);
    uc_egcd(&tmp, &u, &v, &m1, &m2);

    /* Ensure that u is non-negative */
    if ( uc_is_neg(&u) )
    {
        if ( (res = uc_add(&tmp, &u, &m2)) != UC_OK ||
             (res = uc_copy(&u, &tmp)) != UC_OK )
        {
            goto cleanup1;
        }
        assert(!uc_is_neg(&u));
    }

    /* Ensure that v is non-negative */
    if ( uc_is_neg(&v) )
    {
        if ( (res = uc_add(&tmp, &v, &m1)) != UC_OK ||
             (res = uc_copy(&v, &tmp)) != UC_OK )
        {
            goto cleanup1;
        }
        assert(!uc_is_neg(&v));
    }

    /*
     * lambda1 = u * x2 (mod m2)
     * lambda2 = v * x1 (mod m2)
     */
    if ( (res = uc_mul_mod(&lambda1, &u, &x2, &m2)) != UC_OK ||
         (res = uc_mul_mod(&lambda2, &v, &x1, &m1)) != UC_OK )
    {
        goto cleanup1;
    }

    /*
     * x = lambda1 * m1 + lambda2 * m2
     */
    if ( (res = uc_mul(x, &lambda1, &m1)) != UC_OK ||
         (res = uc_mul(&tmp, &lambda2, &m2)) != UC_OK ||
         (res = uc_add(&u, x, &tmp)) != UC_OK ||
         (res = uc_copy(x, &u)) != UC_OK )
    {
        goto cleanup1;
    }

    /* Ensure that x < m1 * m2 */
    uc_mul(&u, &m1, &m2);
    if ( uc_gte(x, &u) )
    {
        if ( (res = uc_sub(&v, x, &u)) != UC_OK ||
             (res = uc_copy(x, &v)) != UC_OK )
        {
            goto cleanup1;
        }
    }

cleanup1:
    uc_free_multi(&tmp, &u, &v, 0, 0, 0);
cleanup2:
    uc_free_multi(&m1, &m2, &x1, &x2, &lambda1, &lambda2);

    return res;
}

/*
 * Conversion
 */

/*
 * Reads an arbitrary-length string in little-endian radix-r representation
 * (2 <= r <= 16) and converts it to a string. the input sting can be prepended
 * with a '+' or '-' sign.
 *
 * Alphabet = 0 1 2 3 4 5 6 7 8 9 A B C D E F where radix r with 2 <= r <= 16 uses
 * the first r letters of the alphabet (lower-case letters are allowed as well).
 */
int uc_read_radix(uc_int *x, const char *y, int radix)
{
    assert(2 <= radix && radix <= 16);
    assert(strlen(y) > 0);

    int sign;
    uc_int tmp;

    /*
    /* Initialize x with zero and ensure that we have enough room
     */
    uc_set_zero(x);
    uc_grow(x, (strlen(y) * radix) / UC_INT_BASE + 1);

    /*
     * Extract the sign and store it. Since at this point x is zero we cannot assign
     * a negative sign; therefore, we assign it at the end of the function.
     */
    sign = UC_POS;
    if ( *y == '+' )
        ++y;
    else if ( *y == '-' )
    {
        sign = UC_NEG;
        ++y;
    }

    uc_init(&tmp);

    for ( ; *y; ++y )
    {
        uc_mul_d(&tmp, x, radix);
        uc_copy(x, &tmp);

        /*
         * Convert input character to digit. The alphabet allows
         * upper and lower case hexadecimal characters.
         */
        uc_digit d;
        if (*y >= '0' && *y <= '9')
            d = *y - '0';
        else if (*y >= 'A' && *y <= 'F')
            d = *y - 'A' + 10;
        else if (*y >= 'a' && *y <= 'f')
            d = *y - 'a' + 10;
        else
            return UC_INPUT_ERR;

        uc_add_d(&tmp, x, d);
        uc_copy(x, &tmp);
    }

    /*
     * If x is zero we do nothing (i.e., we treat the input "+0" and "-0" as "0").
     */
    if ( !uc_is_zero(x) )
        x->sign = sign;

    uc_clamp(x);

    uc_free(&tmp);

    return UC_OK;
}

/*
 * Converts x into a little-endian radix-r string representation. Negative numbers start
 * with a '-' character and the string is null-terminated.
 *
 * The required string length n (which includes the null-byte) can be computed with
 * the helper function uc_write_radix_len(..).
 */
int uc_write_radix(char *s, int n, uc_int *x, int radix)
{
    int res;
    int sign, digit_ctr;
    uc_int xt;                      /* temporary copy of x */
    uc_int q;
    uc_digit r;

    res = UC_OK;

    if ( radix < 2 || radix > 16 )
        return UC_INPUT_ERR;

    /*
     * If x == 0
     */
    if ( uc_is_zero(x) )
    {
        s[0] = '0';
        s[1] = 0;
        return UC_OK;
    }

    if ( (res = uc_init_multi(&xt, &q, 0, 0, 0, 0)) != UC_OK )
        return res;

    if ( (res = uc_copy(&xt, x)) != UC_OK )
        goto cleanup;

    /*
     * If xt is negative, we remember to add a negative sign in the beginning and
     * then proceed with xt := |xt|
     * (We add the negative sign at the end as the string will be reversed)
     */
    sign = UC_POS;
    if (uc_is_neg(&xt) )
    {
        sign = UC_NEG;
        xt.sign = UC_POS;
    }

    /*
     * Create output string in reverse order
     */

    digit_ctr = 0;
    while ( !uc_is_zero(&xt) )
    {
        uc_set_zero(&q);
        uc_div_d(&q, &r, &xt, radix);

        assert(0 <= r && r < radix);
        if (0 <= r && r < 10)
            s[digit_ctr++] = '0' + r;
        else
            s[digit_ctr++] = 'A' + (r - 10);

        uc_copy(&xt, &q);
    }

    /* Append negative sign if necessary */
    if ( sign == UC_NEG )
        s[digit_ctr++] = '-';

    /* Null-terminate string */
    s[digit_ctr] = 0;

    reverse_string(s, digit_ctr);

cleanup:
    uc_free(&xt);
    uc_free(&q);

    return res;
}

/*
 * Converts x into a little-endian radix-r string representation. Negative numbers start
 * with a '-' character and the string is null-terminated.
 *
 * The required string length n (which includes the null-byte) can be computed with
 * the helper function uc_write_radix_len(..).
 *
 * Warning: While the algorithm's theoretical complexity is rather appealing, it runs significantly
 * slower since it relies on MP-division instead of division by a digit. This was tested for integers
 * with up to 16k bits.
 */
int uc_write_radix_slow(char *s, int n, uc_int *x, int radix)
{
    int res;
    int s_len;

    s_len = 0;

    res = _uc_write_radix_slow(s, &s_len, x, radix);
    s[s_len] = 0;
    return res;
}

int _uc_write_radix_slow(char *s, int *n, uc_int *x, int radix)
{
    int res;
    int i, k;
    uc_int rt, tmp1, tmp2;
    uc_int q, r;
    uc_int xt;
    int q_str_len, r_str_len;

    res = UC_OK;

    /* Initialize local variables */
    if ((res = uc_init_multi(&rt, &tmp1, &tmp2, &q, &r, &xt)) != UC_OK )
        return res;

    if ((res = uc_set_i(&rt, radix)) != UC_OK )
        goto cleanup;

    /*
     * We work with a non-negative copy of x (note that x is therefore never
     * negative in recursive function calls). We set the sign at the end.
     */
    if ( (res = uc_copy(&xt, x)) != UC_OK )
        goto cleanup;
    xt.sign = UC_POS;

    /* Recursion base case */
    if ( uc_lt(&xt, &rt) )
    {
        int digit = xt.digits[0];

        assert(0 <= digit && digit < radix);
        if (0 <= digit && digit < 10)
            s[0] = '0' + digit;
        else
            s[0] = 'A' + (digit - 10);

        *n = 1;

        return res;
    }

    /*
     * Find k s.t. radix^(2*k-2) <= A < radix^(2*k)
     *
     * We first approximate k by increasing k exponentially until we overshoot. Afterwards,
     * we reduce k until we have found the right one.
     */

    for ( k = 2; ; k *= 2 )
    {
        if ((res = uc_exp_i(&tmp1, &rt, 2 * k - 2)) != UC_OK )
            goto cleanup;

        if ( uc_gt(&tmp1, &xt) )
            break;
    }

    while ( --k )
    {
        if ((res = uc_exp_i(&tmp1, &rt, 2 * k - 2)) != UC_OK ||
            (res = uc_exp_i(&tmp2, &rt, 2 * k)) != UC_OK )
        {
            goto cleanup;
        }

        if (uc_lte(&tmp1, &xt) && uc_lt(&xt, &tmp2) )
            break;
    }

    /*
     * Compute q and r s.t. A = q * B^k + r
     */
    if ( (res = uc_exp_i(&tmp1, &rt, k)) != UC_OK ||
         (res = uc_div(&q, &r, &xt, &tmp1)) != UC_OK )
    {
        goto cleanup;
    }

    /*
     * Invoke function recursively ond q and r and then combine result as follows:
     *      str(q) || '0'^{k - len(str(r))} || str(r)
     *
     * Note: To avoid memory allocations in recursive invocations, we first compute
     *      str(q) || str(r)
     * and then shift insert the 0's in the middle.
     */

    _uc_write_radix_slow(s, &q_str_len, &q, radix);
    _uc_write_radix_slow(s + q_str_len, &r_str_len, &r, radix);

    int shift = k - r_str_len ;
    rsh_string(s + q_str_len, r_str_len, shift, '0');

    *n = q_str_len + shift + r_str_len; /* Calculate total length */

cleanup:
    uc_free_multi(&rt, &tmp1, &tmp2, &q, &r, &xt);

    return res;
}

/*
 * Returns the string length required (including null-byte)
 * to hold the string representation of x in radix r representation.
 */
int uc_write_radix_len(uc_int *x, int r)
{
    int len;
    uc_int rt, tmp, tmp2;

    if ( uc_is_zero(x) )
        return 2;           /* '0' character and null byte */

    uc_init(&rt);
    uc_init(&tmp);
    uc_init(&tmp2);

    len = 2; /* we need at least 1 digit and a null-byte */

    if ( uc_is_neg(x) )
        ++len; /* one more character for the negative sign '-' */

    uc_init_i(&rt, r);
    uc_init_i(&tmp, r);
    while ( uc_cmp_mag(&tmp, x) == UC_LT )
    {
        ++len;
        uc_mul_d(&tmp2, &tmp, r);
        uc_copy(&tmp, &tmp2);
    }
    ++len; /* Since tmp starts at r, the algorithm above can underestimate the required
            * length by 1 digit. */

    uc_free(&rt);
    uc_free(&tmp);
    uc_free(&tmp2);

    return len;
}

/*
 * Convert byte-encoded integer _bytes_ (big-endian) of length _nbytes_ to integer
 */
int uc_read_bytes(uc_int *x, unsigned char *bytes, int nbytes)
{
    uc_init(x);
    uc_grow(x, (nbytes * 8) / UC_DIGIT_BITS + 1);

    int digit_ctr = 0;
    uc_digit d = 0;
    for ( int i = 0; i < 8 * nbytes; ++i ) // iterate bit-wise over _bytes_
    {
        int b_i = (bytes[i/8] >> (i%8)) & 1;
        d += (b_i << (i % UC_DIGIT_BITS));

        if ((i + 1) % UC_DIGIT_BITS == 0 ) // we have filled up a uc_digit (with UC_DIGIT_BITS bits)
        {
            x->digits[digit_ctr++] = d;
            x->used++;
            d = 0;
        }
    }

    // Add final digit
    x->digits[digit_ctr] = d;
    x->used++;

    uc_clamp(x);

    return UC_OK;
}

/*
 * Miscellaneous / Debug
 */

void uc_debug_print_int(uc_int *x)
{
    int i;

    printf("UC Integer:\n");
    printf("  Used  = %d\n", x->used);
    printf("  Alloc = %d\n", x->alloc);
    printf("  Sign = %d\n", x->sign);
    printf("  [");
    for ( i = 0; i < x->alloc; ++i )
    {
        if ( i < x->used )
            printf("0x%02x, ", x->digits[i]);
        else
            printf("_%02x ", x->digits[i]);
    }
    printf("]\n");
}

void uc_debug_print_int_bytes(uc_int *x)
{
    int i;
    printf("[");
    for ( i = 0; i < x->alloc; ++i )
    {
        if ( i < x->used )
            printf("0x%02x, ", x->digits[i]);
        else
            printf("_%02x ", x->digits[i]);
    }
    printf("]");
    printf("\n");
}

void uc_debug_print_int_radix(uc_int *x, int radix)
{
    assert( 2 <= radix );
    assert( radix <= 16 );

    int len;
    char *s;

    len = uc_write_radix_len(x, radix);
    s = malloc(len * sizeof(char));
    uc_write_radix(s, len, x, radix);
    printf("%s\n", s);
    free(s);
}


