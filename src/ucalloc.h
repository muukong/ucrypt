#ifndef UCRYPT_UCALLOC_H
#define UCRYPT_UCALLOC_H

#include <stdlib.h>

#define XMALLOC(size) malloc((size_t)(size))
#define XFREE(ptr) free((ptr))
#define XCALLOC(nmemb, size) calloc((size_t)(nmemb), (size_t)(size))
#define XREALLOC(ptr, size) realloc((ptr), (size_t)(size))
#define XREALLOCARRAY(ptr, nmemb, size) reallocarray((ptr), (nmemb), (size))

#endif //UCRYPT_UCALLOC_H
