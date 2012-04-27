#include <check.h>

#include <stdlib.h>

#include "tests.h"

int main() {
     int nfailed;
     Suite *s = make_queue_suite();
     SRunner *sr = srunner_create(s);

     srunner_add_suite(sr, make_alsa_suite());
     srunner_add_suite(sr, make_clock_suite());

     srunner_run_all(sr, CK_NORMAL);
     nfailed = srunner_ntests_failed(sr);
     srunner_free(sr);

     return (nfailed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
