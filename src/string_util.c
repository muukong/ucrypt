#include "string_util.h"

void reverse_string(char *s, int len)
{
    int i, j;
    char tmp;

    for ( i = 0, j = len - 1; i < j; ++i, --j )
    {
        tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
    }
}


