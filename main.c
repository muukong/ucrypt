#include <assert.h>
#include <stdio.h>
#include "src/ucalloc.h"
#include "src/integer.h"

int main()
{
    //unsigned char datax[] = "\x13\x90\x35\x36\xb1\xce\xd8\x71\x15\x6c\x54\xb6\x56\xd7\x85\xb1\x02";
    //unsigned char datax[] = "\x1b\x10";

    char datax[] = "6928";



    uc_int x;
    uc_init(&x);
    uc_read_radix(&x, datax, 10);

    //debug_print_bytes(&x);


    return 0;
    /*
    uc_init(&y);


    uc_lshb(&y, &x, 3);
    printf("y = ");
    debug_print_bytes(&y);
     */



    /*
    uc_int x, y, q, r;

    unsigned char datax[] = "\x51\x2c\x5a\x97\x53\xdb\x75\x4f\x66\x48\x6e\x90\xe3\x25\x1d\xc7\xd3\xfe\x53\x3c\xe4\x78";
    unsigned char datay[] = "\x83\x47\xf8\x06\x1a\x39\xbd\xb0\x78\x60\x38\xfa\x38\xdf\x1d\x48\x58\x0d\x01";

    uc_init_from_bytes(&x, datax, sizeof(datax) - 1);
    uc_init_from_bytes(&y, datay, sizeof(datay) - 1);
    uc_init(&q);
    uc_init(&r);

    printf("x = ");
    debug_print_bytes(&x);
    printf("y = ");
    debug_print_bytes(&y);

    uc_div(&q, &r, &x, &y);

    printf("q = ");
    debug_print_bytes(&q);

    printf("r = ");
    debug_print_bytes(&r);
     */

    return 0;
}
