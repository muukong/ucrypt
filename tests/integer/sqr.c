#include <assert.h>
#include <stdio.h>
#include <uctest/minunit.h>
#include <ucrypt/integer.h>
#include <ucrypt/rand.h>

/*
 * Implements some tests for squaring a number. The result is verified using uc_mul which itself
 * is tested before the squaring operation.
 */

#define NITERS 1000

char *test_sqr()
{
    puts("[*] Running squaring tests...");

    int i;
    uc_int x, z, z_, max;

    uc_init_multi(&x, &z, &z_, &max,  0, 0);


    /*
     * Edge Cases
     */

    /* 0^2 == 0 */
    uc_set_zero(&x);
    mu_assert( "Squaring 0 failed: ", uc_sqr(&z, &x) == UC_OK );
    mu_assert("Squaring 0 incorrect result: ", uc_is_zero(&z) == UC_TRUE );

    /* 1^2 == 1 */
    uc_set_i(&x, 1);
    mu_assert( "Squaring 0 failed: ", uc_sqr(&z, &x) == UC_OK );
    mu_assert("Squaring 0 incorrect result: ", uc_is_one(&z) == UC_TRUE );

    /* Squaring single digit */
    uc_set_i(&x, 2345978);
    mu_assert( "Squaring single failed", uc_sqr(&z, &x) == UC_OK );
    uc_mul(&z_, &x, &x);
    mu_assert("Swuaring single digit incorrect result", uc_eq(&z, &z_));

    /*
     * Random tests
     */

    /* Large numbers (tests slow fallback squaring algorithm) */

    /* max = 2^16000 */
    uc_set_i(&max, 1);
    uc_lshb(&max, &max, 16000);

    for ( i = 0; i < NITERS; ++i )
    {
        uc_rand_int(&x, &max);

        mu_assert( "Squaring large random number failed", uc_sqr(&z, &x) == UC_OK );
        uc_mul(&z_, &x, &x);
        mu_assert( "Squaring large random number incorrect result", uc_eq(&z, &z_) );
    }

    /* Medium-sized numbers (tests fast Comba squaring method) */

    /* max = 2^3000 */
    uc_set_i(&max, 1);
    uc_lshb(&max, &max, 3000 );

    for ( i = 0; i < NITERS; ++i )
    {
        uc_rand_int(&x, &max);

        mu_assert( "Squaring large random number failed", uc_sqr(&z, &x) == UC_OK );
        uc_mul(&z_, &x, &x);
        mu_assert( "Squaring large random number incorrect result", uc_eq(&z, &z_) );
    }

    uc_free_multi(&x, &z, &z_, &max, 0, 0);
    return NULL;
}
