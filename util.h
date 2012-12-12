#include <wchar.h>

char *wc2c(wchar_t *wc, int doFree);

#ifndef __USE_GNU
//char *strdup(const char *s);
wchar_t *wcsdup(const wchar_t *s);
#endif

