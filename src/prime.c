#include <stdlib.h>
#include <stdio.h>
#include <ucrypt/integer.h>
#include <ucrypt/prime.h>
#include <ucrypt/rand.h>

/*
 * Returns the number of Miller-Rabin trials for an n-bit number primality check with an error rate
 * of less than 2^{-80}. (See "Average case error estimates for the strong probable prime test by
 * Damgard et. al., and Handbook of Applied Cryptography).
 *
 * Warning: These trial counts are _not_ safe for testing attacker-provided prime candidates (see "Prime
 * and Prejudice: Primality Testing Under Adversarial Conditions").
 */
int miller_rabin_rounds_unsafe(int n)
{
    if ( n >= 1300 )    return 2;
    if ( n >= 850)      return 3;
    if ( n >= 650 )     return 4;
    if ( n >= 550 )     return 5;
    if ( n >= 450 )     return 6;
    if ( n >= 400 )     return 7;
    if ( n >= 350 )     return 8;
    if ( n >= 300 )     return 9;
    if ( n >= 250 )     return 12;
    if ( n >= 200)      return 15;
    if ( n >= 150 )     return 18;
    return 60;
}

/*
 * Small list of prime numbers for trial division. This list was chosen for two reasons:
 * - Approximately 75% of primes can be filtered out, which is good enough (we cannot really go beyond ~80% anyways)
 * - The largest prime in the list (127) fits in a UC digit. We can use this for fast division by a digit
 */
static int TRIAL_PRIMES_COUNT = 31;
static uc_digit TRIAL_PRIMES[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
                                  59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127 };

int uc_is_prime(uc_int *x, int *is_prime, int safe)
{
    int res;
    int t;      /* number of Miller-Rabin trials */

    res = UC_OK;

    /* Init with safe default value */
    *is_prime = UC_FALSE;

    if ( (res = uc_is_prime_trial_division(x, is_prime)) != UC_OK )
        return res;

    /* Trial-division has found a factor; hence, x is not a prime */
    if ( *is_prime == UC_FALSE )
        return res;

    /* Use slow but more precise Miller-Rabin test */
    if ( safe == UC_TRUE )
        t = 60;
    else
        t = miller_rabin_rounds_unsafe(uc_count_bits(x));

    if ( (res = uc_is_prime_miller_rabin(x, is_prime, t)) != UC_OK )
        return res;

    return res;
}

int uc_is_prime_trial_division(uc_int *x, int *is_prime)
{
    int i, res;
    uc_digit r;
    uc_int q, tmp;

    res = UC_OK;
    *is_prime = UC_FALSE;
    if ( uc_init_multi(&q, &tmp, NULL, NULL, NULL, NULL) != UC_OK )
        return res;

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
        if ( uc_init_d(&tmp, TRIAL_PRIMES[i]) != UC_OK )
            return res;

        if ( uc_eq(x, &tmp) )
        {
            *is_prime = UC_TRUE;
            goto cleanup;
        }

        uc_div_d(&q, &r, x, TRIAL_PRIMES[i]);
        if ( r == 0 )
        {
            *is_prime = UC_FALSE;
            goto cleanup;
        }
    }

cleanup:
    uc_free(&q);
    uc_free(&tmp);

    return res;
}

int uc_is_prime_miller_rabin(uc_int *n, int *is_prime, int t)
{
    int i, j;
    uc_int n1, d;
    uc_int a;
    uc_int tmp;
    uc_int x;
    int r;

    *is_prime = UC_FALSE;

    uc_init_multi(&n1, &d, &a, &tmp, &x, 0);

    /* n1 = n - 1 */
    uc_copy(&n1, n);
    uc_sub_d(&n1, &n1, 1);

    /*
     * Write n - 1 as 2^r * d
     */
    r = 0;
    uc_copy(&d, &n1);
    while ( uc_is_even(&d) )
    {
        ++r;
        uc_rshb(&d, &d, 1);
    }

    /* a = 2 */
    uc_set_i(&tmp, 2);

    /* Run t tests */
    for ( i = 0; i < t; ++i )
    {
loop:
        /* Sample a from [2, n-1) */
        uc_rand_int_range(&a, &tmp, &n1);

        uc_exp_mod(&x, &a, &d, n);

        if ( uc_is_one(&x) || uc_eq(&x, &n1) )
            continue;

        for ( j = 0; j < r; ++j )
        {
            uc_sqr(&x, &x);
            uc_mod(&x, &x, n);

            if ( uc_eq(&x, &n1) )
                goto loop;
        }

        *is_prime = UC_FALSE;
        goto cleanup;
    }

    *is_prime = UC_TRUE;

cleanup:
    uc_free_multi(&n1, &d, &a, &tmp, &x, 0);
    return UC_OK;
}
