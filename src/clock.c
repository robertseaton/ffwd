#include <sys/timeb.h>
#include <pthread.h>

#include <assert.h>
#include <stdbool.h>

#include "clock.h"

pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
double clock_initialized_at = -1;
bool clock_initialized = false;

double milliseconds_to_nanoseconds(double milliseconds) {
     return milliseconds * 1000000;
}

double seconds_to_milliseconds(double seconds) {
     return seconds * 1000;
}

double milliseconds_since_epoch() {
      struct timeb current_time;

      ftime(&current_time);

      return seconds_to_milliseconds(current_time.time) + current_time.millitm;
} 

void clock_init() {
     pthread_mutex_lock(&clock_mutex);
     if (clock_initialized == false) {
          clock_initialized_at = milliseconds_since_epoch();
          clock_initialized = true;
     }
     pthread_mutex_unlock(&clock_mutex);
}

double clock_started_at() {
     assert(clock_initialized_at != -1 && "Must call clock_init() before clock_started_at()");

     return clock_initialized_at;
}
