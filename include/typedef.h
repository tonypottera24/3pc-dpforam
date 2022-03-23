#ifndef TYPEDEF_H_
#define TYPEDEF_H_

#include <sys/types.h>

typedef unsigned char uchar;

#define DEBUG

#ifdef DEBUG
#define debug_print(...) fprintf(stderr, __VA_ARGS__)
#else
#define debug_print(...)
#endif

#endif /* TYPEDEF_H_ */
