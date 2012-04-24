#include <check.h>

#include "../src/ao/ao.h"

START_TEST (test_alsa_initialize)
{
     alsa_initialize(48000, 2, AV_SAMPLE_FMT_S16);
}
END_TEST

START_TEST (test_alsa_get_supported_sample_rate)
{
     fail_unless(alsa_get_supported_sample_rate() == 48000, NULL);
}
END_TEST

START_TEST (test_alsa_get_supported_channels)
{
     fail_unless(alsa_get_supported_channels() == 2, NULL);
}
END_TEST

START_TEST (test_alsa_get_supported_av_fmt)
{
     fail_unless(alsa_get_supported_av_fmt() == AV_SAMPLE_FMT_S16, NULL);
}
END_TEST

Suite *make_alsa_suite() {
     Suite *s = suite_create("alsa");
     TCase *tc_get = tcase_create("alsa_get");
     
     tcase_add_test(tc_get, test_alsa_initialize);
     tcase_add_test(tc_get, test_alsa_get_supported_sample_rate);
     tcase_add_test(tc_get, test_alsa_get_supported_channels);
     tcase_add_test(tc_get, test_alsa_get_supported_av_fmt);

     suite_add_tcase(s, tc_get);

     return s;
}
