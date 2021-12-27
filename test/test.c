#include <stdio.h>

#include <ucrypt/integer.h>

int main(int argc, char**argv)
{
    uc_int x, y, z;

    uc_init_multi(&x, &y, &z, 0, 0, 0);

    uc_set_i(&x, 10);
    uc_set_i(&y, 23);
    uc_add(&z, &x, &y);

    uc_debug_print_int_radix(&z, 10);
}
