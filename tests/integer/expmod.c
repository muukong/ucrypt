#include <stdio.h>
#include <time.h>
#include <ucrypt/integer.h>
#include <uctest/inttests.h>
#include <uctest/minunit.h>
#include <uctest/testcase.h>
#include <testdata.h>

#define BUF_LEN (8 * 4096)
#define RADIX 16

char *test_exp_mod()
{
    int res;
    clock_t begin, end;
    double elapsed;
    uc_csv_testcase t;
    char buf[BUF_LEN + 1];
    uc_int x, y, n, z, z_;

    puts("[*] Running expmod tests...");

    uc_init_multi(&x, &y, &n, &z, &z_, 0);

    res = uc_test_init(&t, EXPMOD_TEST_DATA);
    mu_assert("[!] Expmod: Could not read input file.", res != 0);

    elapsed = 0;
    while (uc_testcase_next(&t))
    {
        /* Store test data in x, y, and z_ (note: z_ == x * y) */
        if (uc_testcase_col(&t, 0, buf, BUF_LEN) != 1 ||
            uc_read_radix(&x, buf, RADIX) != UC_OK ||
            uc_testcase_col(&t, 1, buf, BUF_LEN) != 1 ||
            uc_read_radix(&y, buf, RADIX) != UC_OK ||
            uc_testcase_col(&t, 2, buf, BUF_LEN) != 1 ||
            uc_read_radix(&n, buf, RADIX) != UC_OK ||
            uc_testcase_col(&t, 3, buf, BUF_LEN) != 1 ||
            uc_read_radix(&z_, buf, RADIX) != UC_OK)
        {
            mu_assert("[!] Failed to read expmod test data.", 0);
        }

        begin = clock();

        if ( uc_exp_mod(&z, &x, &y, &n) != UC_OK )
            mu_assert("[!] Expmod failed", 0);

        end = clock();

        elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("\t\t... %lf ms\n", 1000 * elapsed);

        if ( !uc_eq(&z, &z_) )
        {
            printf("x = "); uc_debug_print_int_radix(&x, 16);
            printf("y = "); uc_debug_print_int_radix(&y, 16);
            printf("n = "); uc_debug_print_int_radix(&n, 16);
            printf("z = "); uc_debug_print_int_radix(&z, 16);
            printf("z_ = "); uc_debug_print_int_radix(&z_, 16);

            mu_assert("[!] Modexp wrong result", 0);
        }
    }

    uc_testcase_free(&t);
    uc_free_multi(&x, &y, &n, &z, &z_, 0);

    return NULL;
}
