#ifndef CLOCK

typedef struct _Clock* Clock;

double milliseconds_to_nanoseconds(double milliseconds);
double seconds_to_milliseconds(double seconds);
double milliseconds_since_epoch();
void clock_init();
double clock_started_at();

#define CLOCK
#endif
