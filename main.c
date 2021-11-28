#include <stdio.h>
#include "src/ucalloc.h"
#include "src/integer.h"

void test1(void)
{
    uc_int x, y, q, r;

    uc_init_multi(&x, &y, &q, &r, 0, 0);

    //uc_read_radix(&x, "2323459872345976234789234890234587634762389747389238934783465612451242638349348476352363484781234765123765564232121547678967851238123481239184589234565768912345763457623476851234512754765643452334691345347748487461276935768534765347656557642827692457", 10);
    //uc_read_radix(&y, "239047484999999923423467846782465789278651242373849489474623478562347895643322222222222271239613576676152353474383484838933393745634786754234765123478445161161274748484894594574635384756768537685123547812534872147865214786512347851238745123784123794619234617657685785857865543453346234223413423465764786576778484945889076876768527843652780569984845", 10);
    uc_read_radix(&y, "0", 10);

    uc_debug_print_int_radix(&y, 10);

    //uc_div(&q, &r, &x, &y);

    //uc_debug_print_int(&q);
    //uc_debug_print_int_radix(&q, 10);
    //uc_debug_print_int(&r);
    //uc_debug_print_int_radix(&r, 10);
}


void test2(void)
{
    uc_int x, y, q, r;

    uc_init_multi(&x, &y, &q, &r, 0, 0);

    uc_read_radix(&x, "2323459872345976234789234890234587634762389747389238934783465612451242638349348476352363484781234765123765564232121547678967851238123481239184589234565768912345763457623476851234512754765643452334691345347748487461276935768534765347656557642827692457", 10);
    uc_read_radix(&y, "239047484999999923423467745634786754234765123478445161161274748484894594574635384756768537685123547812534872147865214786512347851238745123784123794619234617657685785857865543453346234223413423465764786576778484945889076876768527843652780569984845", 10);

    //uc_debug_print_int_bytes(&x);
    //uc_debug_print_int_bytes(&y);
    uc_div(&q, &r, &x, &y);
    //uc_debug_print_int_bytes(&q);
    //uc_debug_print_int(&r);
    //printf("-------------------\n");
    uc_debug_print_int_radix(&q, 10);
    uc_debug_print_int_radix(&r, 10);
}

void test3(void)
{
    int i, j, k, l, tmp2, q_, r_;
    uc_int x, y, q, r, tmp;

    uc_init_multi(&x, &y, &q, &r, &tmp, 0);
    uc_grow(&x, 4);
    x.used = 4;
    uc_set_i(&y, 10);
    for ( i = 1; i < 128; ++i )
    {
        for ( j = 0; j < 128; ++j )
        {
            for ( k = 0; k < 128; ++k )
            {
                for ( l = 0; l < 128; ++l )
                {
                    x.digits[3] = i;
                    x.digits[2] = j;
                    x.digits[1] = k;
                    x.digits[0] = l;

                    uc_div(&q, &r, &x, &y);

                    tmp2 = 128 * 128 * 128 * i + 128 * 128 * j + 128 * k + l;
                    q_ = tmp2 / 10;
                    r_ = tmp2 % 10;

                    uc_set_i(&tmp, q_);
                    if (!uc_eq_mag(&tmp, &q)) {
                        puts(":(");
                        printf("(i, j, k, l) = (%d, %d, %d, %d)", i, j, k, l);
                        printf("q = ");
                        uc_debug_print_int(&q);
                        printf("x = ");
                        uc_debug_print_int(&x);
                        printf("tmp2 = %d", tmp2);
                        exit(0);
                    } else;//puts(":)");
                }
            }
        }
    }
}

void test4()
{
    const int X = 163841;
    const int Y = 10;
    uc_int x, y, q, r;

    uc_init_multi(&x, &y, &q, &r, 0, 0);

    uc_set_i(&x, X);
    uc_set_i(&y, Y);

    if ( uc_div(&q, &r, &x, &y) != UC_OK )
        printf(":( :(");

    puts("----------------------------");
    uc_debug_print_int(&q);
    uc_debug_print_int(&r);
}

int main(void)
{
    //test1();
    test2();
    //test3();
    //test4();

    return 0;
}
