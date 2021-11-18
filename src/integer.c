#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "integer.h"
#include "ucalloc.h"

int uc_init(uc_int *x)
{
    if ( !x )
        return UC_INPUT_ERR;

    x->digits = NULL;
    x->alloc = 0;
    x->used = 0;
    x->sign = UC_POS;

    return UC_OK;
}

int uc_grow(uc_int *x, int n)
{
    assert( x && n >= 0 );

    int i;
    uc_digit *tmp;

    if (x->alloc < n )
    {
        tmp = XREALLOCARRAY(x->digits, n, sizeof(uc_digit));
        if ( !tmp )
            return UC_MEM_ERR;

        x->digits = tmp;

        // Zero out excess digits
        i = x->alloc;
        x->alloc = n;
        for ( ; i < x->alloc; ++i )
            x->digits[i] = 0;
    }

    return UC_OK;
}

/*
 * Convert byte-encoded integer _bytes_ (big-endian) of length _nbytes_ to integer
 */
int uc_init_from_bytes(uc_int *x, unsigned char *bytes, int nbytes)
{
    uc_grow(x, (nbytes * 8) / DIGIT_BITS + 1);

    int digit_ctr = 0;
    uc_digit d = 0;
    for ( int i = 0; i < 8 * nbytes; ++i ) // iterate bit-wise over _bytes_
    {
        int b_i = (bytes[i/8] >> (i%8)) & 1;
        d += (b_i << (i % DIGIT_BITS));

        if ( (i + 1) % DIGIT_BITS == 0 ) // we have filled up a uc_digit (with DIGIT_BITS bits)
        {
            x->digits[digit_ctr++] = d;
            x->used++;
            d = 0;
        }
    }

    // Add final digit
    x->digits[digit_ctr] = d;
    x->used++;

    // Remove trailing 0's. (TODO: check if this is needed)
    while ( x->digits[x->used-1] == 0 )
        --(x->used);

    return UC_OK;
}

void debug_print(uc_int *x)
{
    int i;

    printf("UC Integer:\n");
    printf("Used  = %d\n", x->used);
    printf("Alloc = %d\n", x->alloc);
    printf("Digits: [");
    for ( i = 0; i < x->alloc; ++i )
    {
        if ( i < x->used )
            printf("0x%02x, ", x->digits[i]);
        else
            printf("_%02x ", x->digits[i]);
    }
    printf("]\n");
}
