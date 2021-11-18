#include <stdio.h>
#include "src/ucalloc.h"
#include "src/integer.h"

int main() {

    uc_int x;
    uc_init(&x);

    unsigned char data[] = "\xff\x00\x98\xE1\xFF\xFF\xEF";
    //unsigned char data[] = "\xff";

    uc_init_from_bytes(&x, data, sizeof(data) - 1);


    debug_print(&x);

    return 0;
}
