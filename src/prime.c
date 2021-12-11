#include "integer.h"
#include "prime.h"
#include "rand.h"

/*
 * Small list of prime numbers for trial division. This list was chosen for two reasons:
 * - Approximately 75% of primes can be filtered out, which is good enough (we cannot really go beyond ~80% anyways)
 * - The largest prime in the list (127) fits in a UC digit. We can use this for fast division by a digit
 */
static int TRIAL_PRIMES_COUNT = 31;
static uc_digit TRIAL_PRIMES[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
                                  59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127 };


int uc_is_prime(uc_int *x, int *is_prime)
{
    return uc_is_prime_trial_division(x, is_prime);
}

int uc_is_prime_trial_division(uc_int *x, int *is_prime)
{
    int i, res;
    uc_digit r;
    uc_int q;

    res = UC_OK;

    if ( uc_is_even(x) || !uc_is_pos(x) || uc_is_one(x) )
    {
        *is_prime = UC_FALSE;
        return UC_OK;
    }

    if ( (res = uc_init(&q)) != UC_OK )
        return res;

    *is_prime = UC_TRUE;
    for ( i = 0; i < TRIAL_PRIMES_COUNT; ++i )
    {
        uc_div_d(&q, &r, x, TRIAL_PRIMES[i]);
        if ( r == 0 )
        {
            *is_prime = UC_FALSE;
            goto cleanup;
        }
    }

cleanup:
    uc_free(&q);

    return res;
}

int uc_is_prime_miller_rabin(uc_int *n, int *is_prime, int t)
{
    int i, j, s, res;
    uc_int a, r, tmp, n1;

    res = UC_OK;

    if ((res = uc_init_multi(&a, &r, &tmp, &n1, 0, 0)) != UC_OK )
        return res;

    /* n1 = n - 1 */
    if ( (res = uc_sub_d(&n1, n, 1)) != UC_OK )
        goto cleanup;

    /*
     * Compute r and s such that n - 1 = 2^s * r
     */
    uc_copy(&r, n);
    uc_sub_d(&r, &r, 1);
    s = 0;
    while ( uc_is_even(&r) )
    {
        uc_div_2(&r, &r);
        ++s;
    }

    /* Run t tests */
    for ( i = 0; i < t; ++i )
    {
        /* Sample a in [2,n-1) */
        if ( (res = uc_set_i(&tmp, 2)) != UC_OK ||
             (res = uc_rand_int_range(&a, &tmp, &n1)) != UC_OK )
        {
            goto cleanup;
        }

        /* a = a^r mod n */
        if ( (res = uc_exp_mod(&a, &a, &r, n)) != UC_OK )
            goto cleanup;

        if ( uc_is_one(&a) || uc_eq(&a, &n1) )
            continue;

        j = 1;
        while ( j <= s - 1 && !uc_eq(&a, &n1) )
        {
            if ( (res = uc_mul_mod(&a, &a, &a, n)) != UC_OK )
                goto cleanup;

            if ( uc_is_one(&a) )
            {
                *is_prime = UC_FALSE;
                goto cleanup;
            }

            ++j;
        }

        if ( !uc_eq(&a, &n1) )
        {
            *is_prime = UC_FALSE;
            goto cleanup;
        }
    }

    *is_prime = UC_TRUE;

cleanup:
    uc_free_multi(&a, &r, &tmp, &n1, 0, 0);

    return res;
}