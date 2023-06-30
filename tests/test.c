#include <stdio.h>
#include <ucrypt/integer.h>
#include <uctest/inttests.h>
#include <uctest/minunit.h>
#include <uctest/hashtests.h>

int tests_run = 0;

static char *all_tests(void)
{
    mu_run_test(test_mul);
    mu_run_test(test_sqr);
    mu_run_test(test_div);
    mu_run_test(test_exp_mod);
    mu_run_test(test_sha224);
    mu_run_test(test_sha256);
    mu_run_test(test_sha384);
    mu_run_test(test_sha512);
    mu_run_test(test_hmac);
    return 0;
}

int main()
{
    char *result = all_tests();

    printf("\n\n\n### Test Summary ###\n");
    if ( result != 0 )
        printf("%s\n", result);
    else
        printf("ALL TESTS PASSED\n");

    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
