#include <sys/timeb.h>

double miliseconds_since_epoch() {
      struct timeb what_time;

      ftime(&what_time);

      return (double)what_time.time * 1000 + what_time.millitm;
} 
