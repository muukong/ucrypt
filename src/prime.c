#include "integer.h"
#include "prime.h"

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
