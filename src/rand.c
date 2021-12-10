#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "integer.h"
#include "rand.h"

/*
 * TODO:
 * - Add non-Linux platforms
 * - Make sure that this runs securely on all Linux platforms
 */

#ifdef __linux__
#   include <sys/random.h>
#endif

/* Linux-specific implementation */

static int _uc_linux_getrandom(void *buf, int n)
{
    int nbytes_read;

    assert(0 < n && n < 256);
    do{
        nbytes_read = getrandom(buf, n, 0);
    } while ( nbytes_read == -1 && (errno == EINTR || errno == EAGAIN));

    return nbytes_read == n ? UC_OK : UC_RNG_ERR;
}

static int uc_linux_getrandom(void *buf, int n)
{
    int res, chunk_size;
    unsigned char *buf_;

    buf_ = buf;
    chunk_size = 256;

    while ( n > 0 )
    {
        if ((res = _uc_linux_getrandom(buf_, n)) != UC_OK )
            return res;

        n -= chunk_size;
        buf_ += chunk_size;
    }

    return UC_OK;
}

/* Generic */

int uc_rand_bytes(void *buf, int n)
{
    return uc_linux_getrandom(buf, n);
}

/*
 * Sample x uniformly in the interval [0, base^k) for k >= 1
 */
int uc_rand_int_base_pow(uc_int *x, int k)
{
    int i, res;

    res = UC_OK;

    /* Make sure k >= 1 */
    if ( k < 1 )
        return UC_INPUT_ERR;

    /* Make sure x can hold result */
    if ((res = uc_grow(x, k)) != UC_OK )
        return res;

    /* Assign random values to digits */
    for ( i = 0; i < k; ++i )
    {
        if ( (res = uc_rand_digit(x->digits + i)) != UC_OK )
            return res;
    }

    /* Zero out remaining digits of x */
    for ( i = 0; i < x->used; ++i )
        x->digits[i] = 0;

    x->used = k;
    x->sign = UC_POS;

    uc_clamp(x);

    return res;
}

/*
 * Sample x in range [0,b) for b > 0
 */
int uc_rand_int(uc_int *x, uc_int *b)
{
    int res;
    uc_int base_power, b_;

    res = UC_OK;

    /* Make sure b > 0 */
    if ( !uc_is_pos(b) )
        return UC_INPUT_ERR;

    uc_init_multi(&b_, &base_power, 0, 0, 0, 0);

    /*
     * We can easily generate numbers in the range [0,base^k) for any integer k >= 0. We therefore
     * find the smallest base power (i.e., base^k) s.t. b < base^k.
     */
    if ( (res = uc_set_w(&base_power, UC_INT_BASE)) != UC_OK )
        goto cleanup;

    while ( uc_lt(&base_power, b) )
    {
        if ( (res = uc_lshd(&base_power, &base_power, 1)) != UC_OK )
            goto cleanup;
    }

    /*
     * Find the largest multiple b_ of b (i.e., b_ = t * b for some positive integer t) with b_ < base^k
     */
    if ( (res = uc_mod(&b_, &base_power, b)) != UC_OK ||
         (res = uc_sub(&b_, &base_power, &b_)) != UC_OK )
    {
        goto cleanup;
    }

    /*
     * Sample uniformly at random in internal [0,base^k) until we find a value x_ in the range [0,b_), which
     * we can then map to the original interval [0,b] with x = x_ % b without loosing uniformity.
     */
    do {
        if ( (res = uc_rand_int_base_pow(x, base_power.used)) != UC_OK )
            goto cleanup;
    } while ( uc_gte(x, &b_) );

    if ( (res = uc_mod(x, x, b)) != UC_OK ) /* Map to interval [0,b) */
        goto cleanup;

    assert(uc_lt(x, b));

cleanup:
    uc_free_multi(&b_, &base_power, 0, 0, 0, 0);

    return res;
}

/*
 * Sample x in range [a,b) for arbitrary integers with a < b
 */
int uc_rand_int_range(uc_int *x, uc_int *a, uc_int *b)
{
    int res;
    uc_int c;

    res = UC_OK;

    /* Make sure that a < b */
    if ( uc_gte(a, b) )
        return UC_INPUT_ERR;

    if ( (res = uc_init(&c)) != UC_OK )
        return res;

    /*
     * Sample random x in range [0,c) for c = a + b
     */
    if ( (res = uc_sub(&c, b, a)) != UC_OK ||
         (res = uc_rand_int(x, &c)) != UC_OK )
    {
        goto cleanup;
    }

    /* Shift x into range [a,b) */
    if ( (res = uc_add(x, x, a)) != UC_OK )
        goto cleanup;

cleanup:
    uc_free(&c);

    return res;
}

/*
 * Sample random integer
 */
int uc_rand_i(int *x)
{
    return uc_rand_bytes(x, sizeof(*x));
}

/*
 * Sample random long integer
 */
int uc_rand_l(long *x)
{
    return uc_rand_bytes(x, sizeof(*x));
}

/*
 * Sample d in range [0,base^k)
 */
int uc_rand_digit(uc_digit *d)
{
    int res;

    res = UC_OK;

#ifdef __linux__

    if ((res = uc_linux_getrandom(d, sizeof(*d))) != UC_OK )
        return res;
    *d &= UC_DIGIT_MASK;

#else
    res = UC_RNG_ERR;
#endif

    return res;
}

