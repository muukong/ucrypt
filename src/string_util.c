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

/*
 * Shift string right by _shift_ places and fill up left with 0's. For example:
 *
 * Shifting "abc" to the right by two with fill character 'x' gives "xxabc"
 */
void rsh_string(char *s, int nbytes, int shift, char c)
{
    int i;

    for ( i = nbytes - 1; i >= 0; --i )
        s[i + shift] = s[i];
    for ( i = 0; i < shift; ++i )
        s[i] = c;
}
