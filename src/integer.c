#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "integer.h"
#include "ucalloc.h"

/*
 * Helper functions that are only used internally
 */

static int _uc_add(uc_int *z, uc_int *x, uc_int *y);
static int _uc_sub(uc_int *z, uc_int *x, uc_int *y);
static int _uc_mul(uc_int *z, uc_int *x, uc_int *y);

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

int uc_copy_int(uc_int *dst, uc_int *src)
{
    uc_grow(dst, src->used);
    uc_zero_out(dst);

    for ( int i = 0; i < src->used; ++i )
        dst->digits[i] = src->digits[i];

    dst->used = src->used;
    dst->sign = dst->sign;

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

    uc_clamp(x);

    return UC_OK;
}

int uc_clamp(uc_int *x)
{
    while ( x->digits[x->used - 1] == 0 )
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
        puts("");
        if ( i % 8 == 0 )
            printf("\n### Byte %d ###\n", i / 8);
        printf("i = %d\n", i);
        printf("n-th bit = %d\n", i + 1);


        //puts("");
        uc_digit b_i = (((uc_word) bytes[i/8]) >> (i%8)) & 1u;
        printf("bytes[i/8] = 0x%02x\n", bytes[i/8]);
        printf("i % 8 == %d\n", i % 8);
        printf("b_i = %d\n", b_i);
        //printf("b_i = %d\n", b_i);
        printf("%d", b_i);
        printf("d = 0x%02x\n", d);
        d += (b_i << (i % DIGIT_BITS));
        printf("d_ = 0x%02x\n", d);

        if ( (i + 1) % DIGIT_BITS == 0 ) // we have filled up a uc_digit (with DIGIT_BITS bits)
        {
            printf("D = 0x%02x\n", d);
            x->digits[digit_ctr++] = d;
            x->used++;
            d = 0;
        }

    }
    puts("");

    // Add final digit
    x->digits[digit_ctr] = d;
    x->used++;

    uc_clamp(x);

    return UC_OK;
}

int uc_zero_out(uc_int *x)
{
    if ( !x->digits )
        uc_grow(x, 1);

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

    assert( uc_cmp_mag(x, y) == UC_GT );

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

int uc_add_d(uc_int *z, uc_int *x, uc_digit y)
{
    uc_int tmp;
    uc_init_from_long(&tmp, y); // TODO: implement "init from digit" function. Otherwiwise, this is a bug.

    uc_add(z, x, &tmp);
    uc_free(&tmp);

    return UC_OK;
}

/*
 * Compute z = x * where where x >= y >= 0
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
        c = tmp >> DIGIT_BITS;
        z->digits[i] = tmp & UC_DIGIT_MASK;
        z->used++;
    }

    for ( ; i < x->used; ++i )
    {
        uc_digit tmp = x->digits[i] + c;
        c = tmp >> DIGIT_BITS;
        z->digits[i] = tmp & UC_DIGIT_MASK;
        z->used++;
    }

    if (c > 0 )
    {
        z->digits[i] = c;
        z->used++;
    }

    return UC_OK;
}

/*
 * Calculate z = x - y for arbitrary UC integers x and y
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

    assert( uc_cmp_mag(x, y) == UC_GT );

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
 * Subtract two integers with |x| >= |y| and x, y >= 0
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

int uc_mul(uc_int *z, uc_int *x, uc_int *y)
{
    int status;

    status = _uc_mul(z, x, y);
    if ( x->sign != y->sign )
        z->sign = UC_NEG;

    return status;
}

int uc_mul_d(uc_int *z, uc_int *x, uc_digit y)
{
    uc_int tmp;
    uc_init(&tmp);
    uc_init_from_long(&tmp, y); // TODO: implement init from digit function - o/w this is a bug

    //uc_mul(z, x, &tmp);

    //uc_free(&tmp);

    return UC_OK;
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
            c = tmp >> DIGIT_BITS;
        }

        z->digits[i + n] = c;
    }

    uc_clamp(z);

    return UC_OK;
}


/*
 * Compute z = x ^ 2
 */
int uc_sqr(uc_int *z, uc_int *x)
{
    // TODO: Implement faster algorithm
    return uc_mul(z, x, x);
}

int uc_div(uc_int *c, uc_int *d, uc_int *a, uc_int *b)
{
    uc_int ta, tb, tq, q;
    int res, n, n2;

    uc_init(&ta);
    uc_init(&tb);
    uc_init(&tq);
    uc_init(&tq);

    uc_init_from_int(&tq, 1);
}

/* Shift y left by n bits and store result in x */
int uc_lshb(uc_int *x, uc_int *y, int n)
{

    printf("y = ");
    debug_print_bytes(y);
    uc_copy_int(x, y);
    printf("x = ");
    debug_print_bytes(x);

    // Ensure that x can hold result
    if ( x->alloc < x->used + (n / DIGIT_BITS) + 1)
        uc_grow(x, x->used + (n / DIGIT_BITS) + 1);

    if ( n >= DIGIT_BITS )
    {
        uc_lshd(x, n / DIGIT_BITS);
        n %= DIGIT_BITS;
    }

    if ( n == 0 )
        return UC_OK;

    for ( int i = x->used - 1; i >= 0; --i )
    {
        uc_digit shift = DIGIT_BITS - n;

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

int uc_abs(uc_int *x, uc_int *y)
{
    int status = uc_copy_int(x, y);
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
 * Modular arithmetic
 */

/*
 * Compute z = x + y (mod m) for 0 <= x < m and 0 <= y < m
 */
int uc_addmod(uc_int *z, uc_int *x, uc_int *y, uc_int *m)
{
    assert(!uc_is_neg(x) && !uc_is_neg(y));
    assert(uc_lt(x, m) && uc_lt(y, m));

    uc_add(z, x, y);
    if (uc_gte(z, m) )
        uc_sub(z, z, m);

    assert(uc_lt(z, m));
}



/*
 * Calculate GCD for two positive integers x and y.
 */
uc_word uc_gcd_word(uc_word x, uc_word y)
{
    return x >= y ? _uc_gcd_word(x, y) : _uc_gcd_word(y, x);
}

/*
 * Calculate GCD for two positive integers with x >= y
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
 * Miscellaneous / Debug
 */

// TODO: move
uc_digit char_to_digit(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return -1;
}

int uc_read_radix(uc_int *x, char *bytes, int radix)
{
    assert(0 <= radix && radix <= 16); // TODO; generalize this

    uc_zero_out(x);

    while ( *bytes )
    {
        uc_int tmp;
        uc_zero_out(&tmp);
        uc_mul_d(&tmp, x, radix);
        //uc_copy_int(x, &tmp);

        uc_digit d = char_to_digit(*bytes);
        printf("d = %d\n", d);
        //uc_add_d(x, x, d);

        ++bytes;
    }

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
void debug_print_bytes(uc_int *x)
{
    int i;
    for ( i = 0; i < x->alloc; ++i )
    {
        if ( i < x->used )
            printf("0x%02x, ", x->digits[i]);
        else
            printf("_%02x ", x->digits[i]);
    }
    printf("]\n");
}
