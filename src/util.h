#ifndef UTIL

double milliseconds_since_epoch();

#define CALL_ONLY_ONCE()                                     \
static int _i = 0;                                            \
assert(_i == 0 && "function called multiple times");          \
_i++

#define UTIL
#endif
