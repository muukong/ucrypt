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

int uc_init_zero(uc_int *x)
{
    uc_init(x);
    uc_grow(x, 1);
    uc_zero_out(x);

    return UC_OK;
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
    assert( n >= 0 );

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
int uc_init_from_int(uc_int *x, int n)
{
    return uc_init_from_long(x, n);
}

/*
 * Initialize UC integer x with built-in long n.
 */
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
 * Initialize UC integer x with digit n.
 */
int uc_init_from_digit(uc_int *x, uc_digit n)
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
 * Convert byte-encoded integer _bytes_ (big-endian) of length _nbytes_ to integer
 */
int uc_init_from_bytes(uc_int *x, unsigned char *bytes, int nbytes)
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
 * Assign zero to x (i.e. x := 0)
 */
int uc_zero_out(uc_int *x)
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
 * Copy y to x (i.e. x := y)
 */
int uc_copy(uc_int *x, uc_int *y)
{
    uc_zero_out(x);

    if ( y->used > x->used )
        uc_grow(x, y->used);

    for ( int i = 0; i < y->used; ++i )
        x->digits[i] = y->digits[i];

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
    uc_zero_out(x);
    XFREE(x->digits);
    x->digits = NULL;
    x->used = 0;
    x->alloc = 0;
    x->sign = UC_POS;

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
 * Compute z = x + y with |x| >= |y| and x, y >= 0
 */
static int _uc_add(uc_int *z, uc_int *x, uc_int *y)
{
    assert(uc_cmp_mag(x, y) != UC_LT);

    /* Ensure that z is initialized with 0 and that there is enough space to hold the result */
    uc_zero_out(z);
    uc_grow(z, x->used + 1);
    z->used = z->used + 1;

    int i;
    uc_digit c = 0;     // carry
    for ( i = 0; i < y->used; ++i )
    {
        uc_digit tmp = x->digits[i] + y->digits[i] + c;
        c = tmp >> UC_DIGIT_BITS;
        z->digits[i] = tmp & UC_DIGIT_MASK;
        z->used++;
    }

    for ( ; i < x->used; ++i )
    {
        uc_digit tmp = x->digits[i] + c;
        c = tmp >> UC_DIGIT_BITS;
        z->digits[i] = tmp & UC_DIGIT_MASK;
        z->used++;
    }

    if (c > 0 )
    {
        z->digits[i] = c;
        z->used++;
    }

    uc_clamp(z);

    return UC_OK;
}

/*
 * Compute z = x + y where x,y UC integers and d digit.
 */
int uc_add_d(uc_int *z, uc_int *x, uc_digit d)
{
    uc_int y;
    uc_init_from_digit(&y, d);
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

    /* Ensure that z is initialized with 0 and that there is enough space to hold the result */
    uc_zero_out(z);
    uc_grow(z, x->used + 1);
    z->used = x->used + 1;

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

    uc_clamp(z);

    return UC_OK;
}

/*
 * Compute z = x * y
 */
int uc_mul(uc_int *z, uc_int *x, uc_int *y)
{
    int status;

    status = _uc_mul(z, x, y);
    if ( x->sign != y->sign )
        z->sign = UC_NEG;

    return status;
}

/*
 * Compute z = |x| * |y|
 */
static int _uc_mul(uc_int *z, uc_int *x, uc_int *y)
{
    int n = x->used;
    int m = y->used;

    /* Allocate enough space, zero out memory, and set minimum number of digits */
    uc_zero_out(z);
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
 * Compute z = x * d for z, x UC integers and digit d.
 *
 * TODO: Use another algorithm to improve speed
 */
int uc_mul_d(uc_int *z, uc_int *x, uc_digit d)
{
    uc_int tmp;
    uc_init_from_digit(&tmp, d);
    int status = uc_mul(z, x, &tmp);
    uc_free(&tmp);
    return status;
}

/*
 * Compute x = 2 * y
 *
 * TODO: Use a more efficient algorithm
 */
int uc_mul_2(uc_int *x, uc_int *y)
{
    uc_int tmp;
    int status = uc_init_from_int(&tmp, 2);
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
        uc_zero_out(q);
        uc_copy(r, x);
        return UC_OK;
    }

    /*
     * If x == y, we know that q = 1 and r = 0;
     * // TODO: check if this case is needed
     */
    if ( uc_eq(x, y) )
    {
        uc_init_from_int(q, 1);
        uc_zero_out(q);
        return UC_OK;
    }

    uc_init(&xt);
    uc_init(&yt);
    uc_init(&tmp);

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
    uc_zero_out(q);
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
        uc_copy(x, &tb);
    }

    /*
     * Steps 2 - 8
     */
    for ( j = m - 1; j >= 0; --j )
    {
        /*
         * Quotient estimation
         */
        q_estimate = ( ((uc_word) x->digits[n+j] * UC_INT_BASE) + ((uc_word) x->digits[n+j-1]) ) / y->digits[n-1];
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

        uc_zero_out(&ta);
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

    return UC_OK;
}

/*
 * Compute x = y // 2, i.e.:
 *   y is even ==> x = y / 2
 *   y odd ==> x = (y - 1) / 2
 */
int uc_div_2(uc_int *x, uc_int *y)
{
    /* Division by two is equivalent to a left shift by one bit */
    return uc_rshb(x, y, 1);
}

/*
 * Compute x = (y << n) for n >= 0
 *
 * I.e., shift y left by n bits and store result in x
 */
int uc_lshb(uc_int *x, uc_int *y, int n)
{
    assert(n >= 0);

    uc_copy(x, y);

    if ( n == 0 ) // Nothing to do
        return UC_OK;

    // Ensure that x can hold result
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

int uc_gcd(uc_int *z, uc_int *x, uc_int *y)
{
    // TODO
}

/*
 * Calculate GCD for two positive integers (uc_word) x and y.
 */
uc_word uc_gcd_word(uc_word x, uc_word y)
{
    return x >= y ? _uc_gcd_word(x, y) : _uc_gcd_word(y, x);
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

    /*
    /* Initialize x with zero and ensure that we have enough room
     */
    uc_zero_out(x);
    uc_grow(x, (strlen(y) * radix) / UC_DIGIT_BITS + 1);

    /*
     * Extract the sign and store it. Since at this point x is zero we cannot assign
     * a negative sign; therefore, we assign it at the end of the function.
     */
    int sign = UC_POS;
    if ( *y == '+' )
        ++y;
    else if ( *y == '-' )
    {
        sign = UC_NEG;
        ++y;
    }

    uc_int tmp;
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
int uc_write_radix(char *y, int n, uc_int *x, int radix)
{
    int sign, digit, digit_ctr;
    uc_int xt;          /* temporary copy of x */
    uc_int q, r, rad;        /* temporary helper variables */

    if ( n < 2 )
        UC_INPUT_ERR;

    /*
     * If x == 0
     */
    if ( uc_is_zero(x) )
    {
        y[0] = '0';
        y[1] = 0;
        return UC_OK;
    }

    uc_init(&xt);
    uc_init(&q);
    uc_init(&r);
    uc_init(&rad);
    digit_ctr = 0;

    uc_copy(&xt, x);
    uc_init_from_int(&rad, radix);

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
    while ( !uc_is_zero(&xt) ) {
        uc_div(&q, &r, &xt, &rad);

        digit = r.digits[0];
        assert(0 <= digit && digit < radix);
        if (0 <= digit && digit < 10)
            y[digit_ctr++] = '0' + digit;
        else
            y[digit_ctr++] = 'A' + (digit - 10);

        uc_copy(&xt, &q);
    }

    /* Append negative sign if necessary */
    if ( sign == UC_NEG )
        y[digit_ctr++] = '-';

    /* Null-terminate string */
    y[digit_ctr] = 0;

    reverse_string(y, digit_ctr);

    uc_free(&xt);
    uc_free(&q);
    uc_free(&r);
    uc_free(&rad);

    return UC_OK;
}

/*
 * Returns the string length required (including null-byte)
 * to hold the string representation of x in radix r representation.
 */
int uc_write_radix_len(uc_int *x, int r)
{
    int len;
    uc_int rt, xt, tmp, tmp2;

    if ( uc_is_zero(x) )
        return 2;           /* '0' character and null byte */

    uc_init(&rt);
    uc_init(&tmp);
    uc_init(&tmp2);

    len = 2; /* we need at least 1 digit and a null-byte */

    if ( uc_is_neg(x) )
        ++len; /* one more character for the negative sign '-' */

    uc_init_from_int(&rt, r);
    uc_init_from_int(&tmp, r);
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
    s = malloc(len * sizeof(char*));
    uc_write_radix(s, len, x, radix);
    printf("%s\n", s);
    free(s);
}


