#include <stdio.h>
#include <ucrypt/integer.h>
#include <uctest/inttests.h>
#include <uctest/minunit.h>

int tests_run = 0;

static char *all_tests(void)
{
    mu_run_test(test_mul);
    mu_run_test(test_div);
    mu_run_test(test_exp_mod);
    return 0;
}

int main()
{
    char *result = all_tests();

    if ( result != 0 )
        printf("%s\n", result);
    else
        printf("ALL TESTS PASSED\n");

    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
