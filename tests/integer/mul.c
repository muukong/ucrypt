#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucrypt/integer.h>
#include <uctest/minunit.h>
#include <uctest/testcase.h>
#include <testdata.h>

#define BUF_LEN (8 * 4096)
#define RADIX 16

char *test_mul()
{
    int res;
    char buf[BUF_LEN + 1];
    uc_csv_testcase t;
    uc_int x, y, z, z_;

    res = uc_test_init(&t, MUL_TEST_DATA);
    mu_assert("[!] Multiplication: Could not read input file.", res != 0);

    uc_init_multi(&x, &y, &z, &z_, 0, 0);

    while ( uc_testcase_next(&t) )
    {
        /* Store test data in x, y, and z_ (note: z_ == x * y) */
        if ( uc_testcase_col(&t, 0, buf, BUF_LEN) != 1 ||
             uc_read_radix(&x, buf, RADIX) != UC_OK ||
             uc_testcase_col(&t, 1, buf, BUF_LEN) != 1 ||
             uc_read_radix(&y, buf, RADIX) != UC_OK ||
             uc_testcase_col(&t, 2, buf, BUF_LEN) != 1 ||
             uc_read_radix(&z_, buf, RADIX) != UC_OK )
        {
            mu_assert("[!] Failed to read multiplication test data.", 0);
        }

        /* Check x * y */
        if ( uc_mul(&z, &x, &y) != UC_OK )
            mu_assert("Error during uc_mul computation", 0);

        if ( !uc_eq(&z, &z_) )
        {
            printf("x = "); uc_debug_print_int_radix(&x, 16);
            printf("y = "); uc_debug_print_int_radix(&y, 16);
            printf("z_ = "); uc_debug_print_int_radix(&z_, 16);
            printf("z = "); uc_debug_print_int_radix(&z, 16);
            mu_assert("[!] uc_mul wrong result (x*y).", 0);
        }

        /* Check y * x */
        if ( uc_mul(&z, &y, &x) != UC_OK )
            mu_assert("Error during uc_mul computation", 0);
        if ( !uc_eq(&z, &z_) )
        {
            printf("x = "); uc_debug_print_int_radix(&x, 16);
            printf("y = "); uc_debug_print_int_radix(&y, 16);
            printf("z_ = "); uc_debug_print_int_radix(&z_, 16);
            printf("z = "); uc_debug_print_int_radix(&z, 16);
            mu_assert("[!] uc_mul wrong result (y*x).", 0);
        }
    }

    uc_testcase_free(&t);
    uc_free_multi(&x, &y, &z, &z_, 0, 0);

    return NULL;
}
