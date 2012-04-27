#include <check.h>

#include "../src/clock.h"

START_TEST (test_milliseconds_to_nanoseconds)
{
     double i, j;

     i = rand();
     j = milliseconds_to_nanoseconds(i);

     fail_unless(j == i * 1000000, NULL);
}
END_TEST

START_TEST (test_seconds_to_milliseconds)
{
     double i, j;

     i = rand();
     j = seconds_to_milliseconds(i);

     fail_unless(j == i * 1000, NULL);

}
END_TEST

START_TEST (test_init_clock)
{
     double start, clock_start;

     start = milliseconds_since_epoch();
     clock_init();
     clock_start = clock_started_at();

     fail_unless(clock_start - start < 10, NULL);
}
END_TEST

START_TEST (test_clock_persistence)
{
     double start, clock_start; 
     
     clock_init();
     start = milliseconds_since_epoch();
     clock_start = clock_started_at();
     clock_init();
     clock_start = clock_started_at();

     fail_unless(start >= clock_start, "Clock doesn't persist over multiple calls.");
}
END_TEST

Suite *make_clock_suite() {
     Suite *s = suite_create("clock");
     TCase *tc_conv = tcase_create("convert");
     TCase *tc_core = tcase_create("core");

     tcase_add_test(tc_conv, test_milliseconds_to_nanoseconds);
     tcase_add_test(tc_conv, test_seconds_to_milliseconds);
     tcase_add_test(tc_core, test_init_clock);
     tcase_add_test(tc_core, test_clock_persistence);

     suite_add_tcase(s, tc_conv);
     suite_add_tcase(s, tc_core);

     return s;
}
