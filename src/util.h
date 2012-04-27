#ifndef UTIL

#define CALL_ONLY_ONCE()                                      \
static int _i = 0;                                            \
assert(_i == 0 && "function called multiple times");          \
_i++

#define UTIL
#endif
