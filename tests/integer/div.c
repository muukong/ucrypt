#include <stdio.h>
#include <ucrypt/integer.h>
#include <uctest/inttests.h>
#include <uctest/minunit.h>
#include <uctest/testcase.h>
#include <testdata.h>

#define BUF_LEN (8 * 4096)
#define RADIX 16

char *test_div()
{
    int res;
    char buf[BUF_LEN + 1];
    uc_csv_testcase t;
    uc_int x, y, q, q_, r, r_;

    puts("[*] Running division tests...");
    res = uc_test_init(&t, DIV_TEST_DATA);
    mu_assert("[!] Division: Could not read input file.", res != 0);

    uc_init_multi(&x, &y, &q, &r, &q_, &r_);

    while ( uc_testcase_next(&t) )
    {
        /* Store test data in x, y, and z_ (note: z_ == x * y) */
        if (uc_testcase_col(&t, 0, buf, BUF_LEN) != 1 ||
            uc_read_radix(&x, buf, RADIX) != UC_OK ||
            uc_testcase_col(&t, 1, buf, BUF_LEN) != 1 ||
            uc_read_radix(&y, buf, RADIX) != UC_OK ||
            uc_testcase_col(&t, 2, buf, BUF_LEN) != 1 ||
            uc_read_radix(&q_, buf, RADIX) != UC_OK ||
            uc_testcase_col(&t, 3, buf, BUF_LEN) != 1 ||
            uc_read_radix(&r_, buf, RADIX) != UC_OK)
        {
            mu_assert("[!] Failed to read division test data.", 0);
        }

        uc_div(&q, &r, &x, &y);
        if ( !uc_eq(&q, &q_) || !uc_eq(&r, &r_) )
        {
            printf("x = "); uc_debug_print_int_radix(&x, 16);
            printf("y = "); uc_debug_print_int_radix(&y, 16);
            printf("q = "); uc_debug_print_int_radix(&q, 16);
            printf("q_ = "); uc_debug_print_int_radix(&q_, 16);
            printf("r = "); uc_debug_print_int_radix(&r, 16);
            printf("r_ = "); uc_debug_print_int_radix(&r_, 16);
            mu_assert("[!] Division failed.", 0);
        }
    }

    uc_testcase_free(&t);
    uc_free_multi(&x, &y, &q, &r, &q_, &r_);

    return NULL;
}
