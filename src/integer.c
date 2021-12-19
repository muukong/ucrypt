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
static int _uc_mul_digs(uc_int *z, uc_int *x, uc_int *y, int digits);
static int _uc_mul_karatsuba(uc_int *C, uc_int *A, uc_int *B, int N);
static int _uc_div(uc_int *q, uc_int *r, uc_int *x, uc_int *y);

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
    return uc_set_l(x, n);
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
    return uc_set_w(x, n);
}

int uc_set_w(uc_int *x, uc_word n)
{
    uc_set_zero(x);
    uc_grow(x, sizeof(n) / sizeof(uc_word) + 2);

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
 * the memory. This function can be invoked on all UC integers that have been
 * initialized with some init function.
 */
int uc_free(uc_int *x)
{
    /* Allow freeing after initializing an integer */
    if ( x->digits == NULL )
        return UC_OK;

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

    return UC_OK;
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

int _uc_mul_digs_comba(uc_int *z, uc_int *x, uc_int *y, int digits)
{
    int res;
    int tx, ty;
    int i, j, i_max, j_max;
    uc_word ws[UC_COMBA_MUL_MAX_DIGS];
    uc_word w;

    res = UC_OK;

    if ( ( res = uc_grow(z, digits)) != UC_OK )
    {
        return res;
    }

    for ( i = 0; i < UC_COMBA_MUL_MAX_DIGS; ++i )
        ws[i] = 0;

    i_max = UC_MIN(digits, x->used + y->used );

    if (i_max > UC_COMBA_MUL_MAX_DIGS )
        return UC_INPUT_ERR;

    w = 0;
    for ( i = 0; i < i_max; ++i ) /* Calculate column by column (i.e. i is column index) */
    {
        tx = UC_MIN(y->used - 1, i);         /* index into x */
        ty = i - tx;                            /* index into y */

        j_max = UC_MIN(x->used - ty, tx + 1);
        for ( j = 0; j < j_max; ++j )
        {
            w += ((uc_word) x->digits[ty + j]) *
                 ((uc_word) y->digits[tx - j]);
        }

        assert(i < UC_COMBA_MUL_MAX_DIGS);
        ws[i] = w % UC_INT_BASE;
        w /= UC_INT_BASE;
    }

    /* Copy ws to z */
    for ( i = 0; i < i_max; ++i )
        z->digits[i] = ws[i];

    /* Zero out remaining digits */
    for ( ; i < z->used; ++i )
        z->digits[i] = 0;

    z->used = i_max;
    uc_clamp(z);

    return res;
}

/*
 * Compute z = x * y with Karatsuba method.
 *
 * Warning:
 * - This naive implementation is slower than uc_mul, even for very large inputs
 * (e.g. 15k bits).
 * - Not safe for input aliasing
 * TODO: Improve performance.
 */
int uc_mul_karatsuba(uc_int *z, uc_int *x, uc_int *y)
{

    if ( x->used != y->used )
        return UC_INPUT_ERR;

    return _uc_mul_karatsuba(z, x, y, 50);
}

int _uc_mul_karatsuba(uc_int *C, uc_int *A, uc_int *B, int N)
{
    int k, n, res;
    int sign_A, sign_B;
    uc_int tmp1, tmp2;
    uc_int a0, a1, b0, b1;
    uc_int c0, c1, c2;

    res = UC_OK;

    n = A->used;

    /* Base case */
    if ( n < N )
    {
        return uc_mul(C, A, B);
    }

    uc_init_multi(&tmp1, &tmp2, &a0, &a1, &b0, &b1);
    uc_init_multi(&c0, &c1, &c2, 0, 0, 0);

    k = n / 2;

    /* Compute Base^k */
    //uc_set_i(&tmp1, 1);
    //uc_lshd(&tmp1, &tmp1, k);

    /*
     * Split a and b as follows:
     * a = a1 * base^k + a0
     * b = b1 * base^k + b0
     */
    uc_mod_base_pow(&a0, A, k);
    uc_rshd(&a1, A, k);
    uc_mod_base_pow(&b0, B, k);
    uc_rshd(&b1, B, k);

    uc_sub(&tmp1, &a0, &a1);
    uc_sub(&tmp2, &b0, &b1);

    sign_A = tmp1.sign;
    tmp1.sign = UC_POS;
    sign_B = tmp2.sign;
    tmp2.sign = UC_POS;

    _uc_mul_karatsuba(&c0, &a0, &b0, N);
    _uc_mul_karatsuba(&c1, &a1, &b1, N);
    _uc_mul_karatsuba(&c2, &tmp1, &tmp2, N);

    uc_copy(C, &c0);

    uc_add(&tmp1, &c0, &c1);
    if ( sign_A == sign_B )
        uc_sub(&tmp1, &tmp1, &c2);
    else
        uc_add(&tmp1, &tmp1, &c2);
    uc_lshd(&tmp1, &tmp1, k);
    uc_add(C, C, &tmp1);

    uc_copy(&tmp1, &c1);
    uc_lshd(&tmp1, &tmp1, 2 * k);
    uc_add(C, C, &tmp1);


//cleanup:
    uc_free_multi(&tmp1, &tmp2, &a0, &a1, &b0, &b1);
    uc_free_multi(&c0, &c1, &c2, 0, 0, 0);

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

int uc_sub_d(uc_int *z, uc_int *x, uc_digit y)
{
    int res;
    uc_int yt;

    res = UC_OK;

    if ( (res = uc_init(&yt)) != UC_OK )
        return res;

    if ( (res = uc_set_d(&yt, y)) != UC_OK ||
         (res = uc_sub(z, x, &yt)) != UC_OK )
    {
        goto cleanup;
    }

cleanup:
    uc_free(&yt);

    return res;
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
    return uc_mul_digs(z, x, y, x->used + y->used);
}

/*
 * Compute z = x * y (mod BASE^digits)
 */
int uc_mul_digs(uc_int *z, uc_int *x, uc_int *y, int digits)
{
    int res;
    uc_int xt, yt;

    res = UC_OK;

    if ( (res = uc_init_multi(&xt, &yt, 0, 0, 0, 0)) != UC_OK )
        return res;

    if ( (res = uc_copy(&xt, x)) != UC_OK ||
         (res = uc_copy(&yt, y)) != UC_OK )
    {
        goto cleanup;
    }

    if ((UC_MIN(digits, UC_MIN(x->used, y->used))) <= UC_COMBA_MUL_MAX_DIGS )
    {
        /* We can use faster Comba multiplication */
        if ( (res = _uc_mul_digs_comba(z, &xt, &yt, digits)) != UC_OK )
            goto cleanup;
    }
    else
    {
        /* Fallback to slow multiplication */
        if ( (res = _uc_mul_digs(z, &xt, &yt, digits)) != UC_OK )
            goto cleanup;
    }

    if ( x->sign != y->sign )
        z->sign = UC_NEG;

cleanup:
    uc_free(&xt);
    uc_free(&yt);

    return res;
}

/*
 * Compute z = |x| * |y| (mod BASE^digits)
 *
 * Warning: z must not be aliased with x or y
 */
static int _uc_mul_digs(uc_int *z, uc_int *x, uc_int *y, int digits)
{
    int i, j, j_max, c;
    int res;

    /* Make sure z can hold result and set z = 0 */
    if ( (res = uc_grow(z, digits)) != UC_OK ||
         (res = uc_set_zero(z)) != UC_OK )
    {
        return res;
    }

    for ( i = 0; i < x->used; ++i )
    {
        j_max = UC_MIN(y->used, digits - i);

        if ( j_max < 1 ) /* We are done; all remaining products would be larger than base^digits */
            break;

        c = 0; /* carry */
        for ( j = 0; j < j_max; ++j )
        {
            /* tmp = r_ij + x_j * y_i + c */
            uc_word tmp = ((uc_word) z->digits[i + j]) +
                          ((uc_word) x->digits[j]) *
                          ((uc_word) y->digits[i]) +
                          ((uc_word) c);

            z->digits[i + j] = tmp & UC_DIGIT_MASK;
            c = tmp >> UC_DIGIT_BITS;
        }

        if ( i + j < digits )
            z->digits[i+j] = c;
    }

    z->used = digits;
    uc_clamp(z);

    return res;
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
 */
int uc_mul_2(uc_int *x, uc_int *y)
{
    return uc_lshb(x, y, 1);
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

int uc_div(uc_int *q, uc_int *r, uc_int *x, uc_int *y)
{
#ifdef UC_SCHOOL_SMALL_DIV
    return uc_div_school_small(q, r, x, y);
#else
    return uc_div_school_fast(q, r, x, y);
#endif
}

/*
 * Compute r and q s.t. x = q * y + r for non-negative integers x and y.
 */
int uc_div_school_small(uc_int *q, uc_int *r, uc_int *x, uc_int *y)
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

int uc_div_school_fast(uc_int *q, uc_int *r, uc_int *x, uc_int *y)
{
    int res;
    int norm;
    int i, j;
    int n, t;
    uc_int xt, yt, tmp, tmp2;

    res = UC_OK;

    /* Division by 0? No can do... */
    if ( uc_is_zero(y) )
        return UC_INPUT_ERR;

    /* If x = y, q = 1 and r = 0 */
    if ( uc_eq(x, y) )
    {
        if ( (res = uc_set_i(q, 1)) != UC_OK )
            return res;
        return uc_set_zero(r);
    }

    /* If x < y, set q = 0 and r = x */
    if ( uc_lt(x, y) )
    {
        if ( (res = uc_set_zero(q)) != UC_OK ||
             (res = uc_copy(r, x)) != UC_OK )
        {
            return res;
        }
    }

    /* Initialize local variables */
    uc_init_multi(&xt, &yt, &tmp, &tmp2, 0, 0);

    /* Copy inputs to temporary variables */
    if ( (res = uc_copy(&xt, x)) != UC_OK ||
         (res = uc_copy(&yt, y)) != UC_OK )
    {
        goto cleanup;
    }

    /* Normalize */

    norm = uc_count_bits(&yt) % UC_DIGIT_BITS;
    if (norm <= (UC_DIGIT_BITS - 1))
    {
        norm = (UC_DIGIT_BITS - 1) - norm + 1;
        if ( (res = uc_lshb(&xt, &xt, norm)) != UC_OK ||
             (res = uc_lshb(&yt, &yt, norm)) != UC_OK )
        {
            goto cleanup;
        }
    }
    else
        norm = 0;

    n = xt.used - 1;
    t = yt.used - 1;

    assert(yt.digits[t] >= UC_INT_BASE / 2); /* check normalization condition */

    /* Make sure q and r can hold result */
    uc_grow(q, n - t + 1);
    uc_grow(r, t + 1);

    /* Step 1 */

    for ( j = 0; j <= n - t; ++j )
        q->digits[j] = 0;

    /* Step 2 */

    if ( (res = uc_copy(&tmp, &yt)) != UC_OK ||
         (res = uc_lshd(&tmp, &tmp, n - t)) != UC_OK )
    {
        goto cleanup;
    }

    while ( uc_gt(&xt, &tmp) )
    {
        q->digits[n-t] += 1;
        if ( (res = uc_sub(&xt, &xt, &tmp)) != UC_OK )
            goto cleanup;
    }

    /* Step 3 */
    for ( i = n; i >= t + 1; --i )
    {
        /* Step 3.1 */

        if ( xt.digits[i] == yt.digits[t])
        {
            q->digits[i-t-1] = UC_INT_BASE - 1;
        }
        else
        {
            uc_word acc = (uc_word) xt.digits[i];
            acc *= UC_INT_BASE;
            acc += xt.digits[i-1];
            acc /= yt.digits[t];
            q->digits[i-t-1] = acc;
        }

        /* Step 3.2 */

        if ( (res = uc_grow(&tmp, 2)) != UC_OK ||
             (res = uc_grow(&tmp2, 3)) != UC_OK )
        {
            goto cleanup;
        }

        while ( 1 )
        {
            /* Left-hand side tmp1 */
            if ( (res = uc_set_zero(&tmp)) != UC_OK )
                goto cleanup;
            tmp.digits[0] = (t - 1 < 0) ? 0 : yt.digits[t-1];
            tmp.digits[1] = yt.digits[t];
            tmp.used = 2;
            if ( (res = uc_mul_d(&tmp, &tmp, q->digits[i-t-1])) != UC_OK )
                goto cleanup;

            /* Right-hand side tmp2 */
            if ( (res = uc_set_zero(&tmp2)) != UC_OK )
                goto cleanup;
            tmp2.digits[0] = (t - 2 < 0) ? 0 : xt.digits[i-2];
            tmp2.digits[1] = xt.digits[i-1]; /* loop invariant: i >= 1 */
            tmp2.digits[2] = xt.digits[i];
            tmp2.used = 3;

            if ( !uc_gt(&tmp, &tmp2) )
                break;

            q->digits[i-t-1] -= 1;
        }

        /* Step 3.3 */

        if ( (res = uc_copy(&tmp, &yt)) != UC_OK ||
             (res = uc_mul_d(&tmp, &tmp, q->digits[i - t - 1])) != UC_OK ||
             (res = uc_lshd(&tmp, &tmp, i - t - 1)) != UC_OK ||
             (res = uc_sub(&xt, &xt, &tmp)) != UC_OK )
        {
            goto cleanup;
        }

        /* Step 3.4 */

        if ( uc_is_neg(&xt) )
        {
            if ( (res = uc_copy(&tmp, &yt)) != UC_OK ||
                 (res = uc_lshd(&tmp, &tmp, i - t - 1)) != UC_OK ||
                 (res = uc_add(&xt, &xt, &tmp)) != UC_OK )
            {
                goto cleanup;
            }

            q->digits[i - t - 1] -= 1;
        }
    }

    if ( (res = uc_copy(r, &xt)) != UC_OK ||
         (res = uc_rshb(r, r, norm)) != UC_OK )
    {
        goto cleanup;
    }

    q->used = q->alloc;
    uc_clamp(q);

cleanup:
    uc_free_multi(&xt, &yt, &tmp, &tmp2, 0, 0);

    return res;
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
    int j;
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
    uc_lshd(&ta, &ta, m);

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
        uc_lshd(&ta, &ta, j);                // ta = ta * (base ^ j)
        uc_sub(&tb, x, &ta);
        uc_copy(x, &tb);

        uc_set_zero(&ta);
        while ( uc_lt(x, &ta) )
        {
            q->digits[j]--;
            uc_copy(&tb, y);
            uc_lshd(&tb, &tb, j);
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

    /* Cannot divide by zero */
    if ( y == 0 )
        return UC_INPUT_ERR;

    /* Can take shortcut if x is zero */
    if ( uc_is_zero(x) )
    {
        *r = 0;
        return uc_set_zero(q);
    }

    /* Make sure q can hold result */
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
    uc_int xt, yt, tmp;
    int i, n, res;

    res = UC_OK;

    if ( uc_is_neg(y) )
        return UC_INPUT_ERR;

    /*
     * If y is zero, simply set z to 1.
     *
     * Note: We use the convention that 0^0 = 1.
     */
    if ( uc_is_zero(y) )
        return uc_set_i(z, 1);

    if ( (res = uc_init_multi(&xt, &yt, &tmp, 0, 0, 0)) != UC_OK )
        return res;

    /* Make copies of x and y to avoid aliasing */
    if ( (res = uc_copy(&xt, x)) != UC_OK ||
         (res = uc_copy(&yt, y)) != UC_OK )
    {
        goto cleanup;
    }

    if ( (res = uc_init(&tmp)) != UC_OK ||
         (res = uc_set_i(z, 1)) != UC_OK )
    {
        goto cleanup;
    }

    /*
     * Square and multiply. (Note: we ignore the sign during this loop and
     * fix it later).
     */
    n = uc_count_bits(&yt);
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

        if ( (res = uc_mul(&tmp, z, &xt)) != UC_OK )
            goto cleanup;

        if ( uc_nth_bit(&yt,i) == 1 )
            uc_copy(z, &tmp);
        else
            uc_copy(z, z);
    }

    /* Fix sign */
    if ( uc_is_neg(&xt) && uc_is_odd(&yt) )
        z->sign = UC_NEG;
    else
        z->sign = UC_POS;

cleanup:
    uc_free_multi(&xt, &yt, &tmp, 0, 0, 0);

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
        uc_lshd(x, x, n / UC_DIGIT_BITS);
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
    int res;

    res = UC_OK;

    /* Negative shifts are not allowed */
    if ( n < 0 )
        return UC_INPUT_ERR;

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
    if ( (res = uc_copy(x, y)) != UC_OK )
        goto cleanup;

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
        if ( (res = uc_rshd(x, x, n / UC_DIGIT_BITS)) != UC_OK )
            goto cleanup;
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

    res = uc_clamp(x);

cleanup:
    return res;
}

/*
 * Shift y left by n >= digits.
 *
 * For example, if n = 3:
 * [x_0 x_1 x_2 x_3 ... x_{n-1}] --> [0 0 0 x_0 x_1 x_2 x_3 ... x_{n-1}]
 * (Note: The array above depicts the actual internal representation in y->digits)
 */
int uc_lshd(uc_int *x, uc_int *y, int n)
{
    int i, res, oldused;

    res = UC_OK;

    /* Negative shifts are not allowed */
    if (n < 0 )
        return UC_INPUT_ERR;

    /* No shifting necessary */
    if (n == 0 )
        return uc_copy(x, y);

    /*
     * Nothing to do if y is zero.
     *
     * Warning: Do not remove this check unless you clamp the number at the very end (otherwise
     * you end up with a non-normalized zero).
     */
    if ( uc_is_zero(y) )
        return uc_set_zero(x);

    /* Make sure x can hold result */
    if ((res = uc_grow(x, y->used + n)) != UC_OK )
        return res;

    /* Iterate backwards (i.e. from x_{n-1} to x_0) and copy x_i to x_{i+n} */
    for ( i = y->used - 1; i >= 0; --i )
        x->digits[i + n] = y->digits[i];

    /* Zero out the first n digits */
    for ( i = 0; i < n; ++i )
        x->digits[i] = 0;

    oldused = x->used;
    x->used = y->used + n;

    /* Zero out most significant digits (if available) */
    for ( i = x->used; i < oldused; ++i )
        x->digits[i] = 0;

    return res;
}

/*
 * Shift x right by y >= 0 digits.
 *
 * For example, if y = 3:
 * [x_0 x_1 x_2 x-3 ... x_n] --> [x_3 x_4 ... x_n 0 0 0]
 * (Note: The array above depicts the actual internal representation in x->digits)
 */
int uc_rshd(uc_int *z, uc_int *x, int y)
{
    int i, res;

    res = UC_OK;

    /* Negative shifts are not allowed */
    if ( y < 0 )
        return UC_INPUT_ERR;

    /* No shifting required */
    if ( y == 0 )
        return uc_copy(z, x);

    /* Warning: removing this check can lead to an array underflow later */
    if ( y >= x->used )
        return uc_set_zero(z);

    /*
     * Nothing to do if x is zero.
     *
     * Warning: Do not remove this check unless you clamp the number at the very end (otherwise
     * you end up with a non-normalized zero).
     */
    if ( uc_is_zero(x) )
        return uc_set_zero(z);

    if ( (res = uc_grow(z, x->used - y)) != UC_OK )
        return res;

    for ( i = 0; i < x->used - y; ++i )
        z->digits[i] = x->digits[i+y];

    z->used = x->used - y;
    for ( ; i < z->used; ++i )
        z->digits[i] = 0;

    return res;
}

/*
 * x = |y|
 */
int uc_abs(uc_int *x, uc_int *y)
{
    int res;

    res = UC_OK;

    if ( (res = uc_copy(x, y)) != UC_OK )
        return res;
    x->sign = UC_POS;

    return res;
}

int uc_flip_sign(uc_int *x)
{
    /* Do nothing if x is zero ("-0" is not allowed) */
    if ( uc_is_zero(x) )
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

    if (uc_is_zero(x) )
        return 1;

    int nbits = (x->used - 1) * UC_DIGIT_BITS;

    /*
     * Let the most significant digit be [d_0, d_1, ..., d_{UC_DIGIT_BITS-1}]. We look for the largest i
     * (in descending order) s.t. d_i != 0 and then add (i+1) to the current count.
     */
    for (int i = UC_DIGIT_BITS - 1; i >= 0; --i )
    {
        if ( x->digits[x->used-1] & ((uc_digit)1 << i) )
        {
            nbits += (i + 1);
            break;
        }
    }

    return nbits;
}

/*
 * Modular arithmetic
 */

/*
 * Compute z = (x + y) (mod m) for 0 <= x,y < m
 */
int uc_add_mod(uc_int *z, uc_int *x, uc_int *y, uc_int *m)
{
    int res;
    uc_int mt;

    res = UC_OK;

    /* check that 0 <= x < m */
    if (uc_is_neg(x) || !uc_lt(x, m) )
        return UC_INPUT_ERR;

    /* check that 0 <= y < m */
    if (uc_is_neg(y) || !uc_lt(y, m) )
        return UC_INPUT_ERR;

    /* At this point we implicitly know that m > 0 */

    /* Make temporary copy of m */
    if ( (res = uc_init(&mt)) != UC_OK ||
         (res = uc_copy(&mt, m)) != UC_OK )
    {
        return res;
    }

    /*
     * Compute z = x + y and then subtract m if z > m. Afterwards, z < m since x, y < m.
     */
    if ( (res = uc_add(z, x, y)) != UC_OK )
        goto cleanup;
    if ( uc_gte(z, &mt) )
    {
        if ( (res = uc_sub(z, z, &mt)) != UC_OK )
            goto cleanup;
    }

cleanup:
    uc_free(&mt);

    return res;
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
 * Compute z = x ^ y (mod m) for 0 <= x, y < m
 *
 * // TODO: Add input validation
 */
int uc_exp_mod(uc_int *z, uc_int *x, uc_int *y, uc_int *m)
{
    uc_int xt, yt, tmp;
    int i, n, res;

    res = UC_OK;

    if ( uc_is_neg(y) )
        return UC_INPUT_ERR;

    /*
     * If y is zero, simply set z to 1.
     *
     * Note: We use the convention that 0^0 = 1.
     */
    if ( uc_is_zero(y) )
        return uc_set_i(z, 1);

    if ( (res = uc_init_multi(&xt, &yt, &tmp, 0, 0, 0)) != UC_OK )
        return res;

    /* Make copies of x and y to avoid aliasing */
    if ( (res = uc_copy(&xt, x)) != UC_OK ||
         (res = uc_copy(&yt, y)) != UC_OK )
    {
        goto cleanup;
    }

    if ( (res = uc_init(&tmp)) != UC_OK ||
         (res = uc_set_i(z, 1)) != UC_OK )
    {
        goto cleanup;
    }

    /*
     * Square and multiply. (Note: we ignore the sign during this loop and
     * fix it later).
     */
    n = uc_count_bits(&yt);
    for ( i = n - 1; i >= 0; --i )
    {
        /* z = z * z */
        if ( (res = uc_mul_mod(&tmp, z, z, m)) != UC_OK ||
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

        if ( (res = uc_mul_mod(&tmp, z, &xt, m)) != UC_OK )
            goto cleanup;

        if ( uc_nth_bit(&yt,i) == 1 )
            uc_copy(z, &tmp);
        else
            uc_copy(z, z);
    }

    /* Fix sign */
    if ( uc_is_neg(&xt) && uc_is_odd(&yt) )
        z->sign = UC_NEG;
    else
        z->sign = UC_POS;

    cleanup:
    uc_free_multi(&xt, &yt, &tmp, 0, 0, 0);

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
    uc_int c, w, q, r, yt, tmp, mt;

    res = UC_OK;

    if ( (res = uc_init_multi(&c, &tmp, &w, &q, &r, &yt)) != UC_OK ||
         (res = uc_init(&mt)) != UC_OK )
    {
        return res;
    }

    /* Make local copy of y */
    if ( (res = uc_copy(&yt, y)) != UC_OK )
        goto cleanup;

    /* Make local copy of mt */
    if ( (res = uc_copy(&mt, m)) != UC_OK )
        goto cleanup;

    if ( (res = uc_set_i(x, 1)) != UC_OK ||
         (res = uc_set_zero(&w)) != UC_OK ||
         (res = uc_copy(&c, &mt)) != UC_OK )
    {
        goto cleanup;
    }

    while ( !uc_is_zero(&c) )
    {
        if ( (res = uc_div(&q, &r, &yt, &c)) != UC_OK )
            goto cleanup;

        /*
         * (y, c) := (c, r)
         */

        if ( (res = uc_copy(&yt, &c)) != UC_OK ||
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

        if ( (res = uc_mul_mod(&tmp, &q, &w, &mt)) != UC_OK ||
             (res = uc_sub(&w, x, &tmp)) != UC_OK )
        {
            goto cleanup;
        }

        /* If w < 0, add m to make it non-negative again */
        if ( uc_is_neg(&w) )
        {
            uc_add(&tmp, &w, &mt);
            uc_copy(&w, &tmp);
        }
        assert( !uc_is_neg(&w) );

        if ( (res = uc_copy(x, &r)) != UC_OK )
            goto cleanup;
    }

cleanup:
    uc_free(&mt);
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
        return UC_INPUT_ERR;

    /* No reductions needed if y < m */
    if ( uc_lt(y, m) )
        return uc_copy(x, y);

    if ( (res = uc_init(&qt)) != UC_OK )
        return res;

    if ( (res = uc_div(&qt, x, y, m)) != UC_OK )
        goto cleanup;

cleanup:
    uc_free(&qt);

    return res;
}

/*
 * Compute x = y (mod base^k) for y >= 0
 */
int uc_mod_base_pow(uc_int *x, uc_int *y, int k)
{
    int i, res;

    if ( uc_is_neg(y) )
        return UC_INPUT_ERR;

    if ( (res = uc_grow(x, k)) != UC_OK )
        goto cleanup;

    for ( i = 0; i < k; ++i )
        x->digits[i] = y->digits[i];

    for ( ; i < x->used; ++i )
        x->digits[i] = 0;

    x->sign = UC_POS;
    x->used = k;
    uc_clamp(x);

cleanup:
    return res;
}


int uc_gcd(uc_int *z, uc_int *x, uc_int *y)
{
    int res;
    uc_int yt, tmp;

    if ( !uc_is_pos(x) || !uc_is_pos(y) )
        return UC_INPUT_ERR;

    res = UC_OK;

    if ( (res = uc_init_multi(&yt, &tmp, 0, 0, 0, 0)) != UC_OK )
        return res;

    /*
     * Make local copy of y and then copy x to z.
     *
     * Note: We need not create a copy of x since we do not access it after this.
     * Warning: Changing the order of these two operations leads to input aliasing.
     */
    if ( (res = uc_copy(&yt, y)) != UC_OK ||
         (res = uc_copy(z, x)) != UC_OK )
    {
        goto cleanup;
    }

    while ( !uc_is_zero(&yt) )
    {
        if ( (res = uc_copy(&tmp, z)) != UC_OK ||
             (res = uc_copy(z, &yt)) != UC_OK ||
             (res = uc_mod(&yt, &tmp, &yt)) != UC_OK )
        {
            goto cleanup;
        }
    }

cleanup:
    uc_free(&yt);
    uc_free(&tmp);

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

    /* Warning: Changing the order of these two operations leads to input aliasing */
    if ( (res = uc_copy(&bt, b)) != UC_OK ||
         (res = uc_copy(g, a) != UC_OK) )
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

int uc_lcm(uc_int *z, uc_int *x, uc_int *y)
{
    int res;
    uc_int tmp1, tmp2, tmp3;

    if ( (res = uc_init_multi(&tmp1, &tmp2, &tmp3, 0, 0, 0)) != UC_OK )
        return res;

    if ( (res = uc_gcd(&tmp1, x, y)) != UC_OK ||
         (res = uc_mul(&tmp2, x, y)) != UC_OK ||
         (res = uc_div(z, &tmp3, &tmp2, &tmp1)) != UC_OK )
    {
        goto cleanup;
    }

cleanup:
    uc_free_multi(&tmp1, &tmp2, &tmp3, 0, 0, 0);

    return res;
}

uc_word uc_lcm_w(uc_word x, uc_word y)
{
    int gcd;

    gcd = uc_gcd_word(x, y);
    return (x * y) / gcd;
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
     * Initialize x with zero and ensure that we have enough room
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
int uc_write_radix(char *s, uc_int *x, int radix)
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

        assert( r < (uc_digit) radix);
        if ( r < 10 ) /* Note: r >= 0 is implicit since it is unsigned */
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
            printf("0x%02lx, ", x->digits[i]);
        else
            printf("_%02lx ", x->digits[i]);
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
            printf("0x%02lx, ", x->digits[i]);
        else
            printf("_%02lx ", x->digits[i]);
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
    uc_write_radix(s, x, radix);
    printf("%s\n", s);
    free(s);
}


