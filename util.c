// This file contains utility functions.

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "util.h"


char *wc2c(wchar_t *wc, int doFree) {
	const wchar_t *wc2 = wc;
	size_t clen = wcslen(wc) * 4;
	if (clen < 32) clen = 32;
	char *cret = (char*)malloc(clen);
	if (wcsrtombs (cret, &wc2, clen, NULL) == -1) {
		strcpy(cret, "[wide character conversion failed]");
	}
	if (doFree) free(wc);
	return cret;
}

#ifndef __USE_GNU

// These functions are common extensions but we are compiling
// in C99 mode.

// Duplicate a character string, return the new string.
/*
char *strdup(const char *s) {
	char *t = (char*)calloc(strlen(s) + 1, sizeof(char));
	strcpy(t, s);
	return t;
}
*/

// Duplicate a wide character string, return the new string.
wchar_t *wcsdup(const wchar_t *s) {
	wchar_t *t = (wchar_t*)calloc(wcslen(s) + 1, sizeof(wchar_t));
	wcscpy(t, s);
	return t;
}

#endif
