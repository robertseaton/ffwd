#include <stdlib.h>
#include <check.h>
#include <pthread.h>
#include "../src/queue.h"

pthread_cond_t is_full = PTHREAD_COND_INITIALIZER;

START_TEST (test_push_pop)
{
     PacketQueue q;
     AVPacket pkt, pkt1;
     q = queue_create();

     pkt.pts = 1337;
     pkt.data = "SOMETHING";

     queue_push(q, &pkt);
     queue_pop(q, &pkt1);
     
     fail_unless(pkt1.pts == 1337, NULL);
}
END_TEST

START_TEST (test_multiple_push_pop)
{
     PacketQueue q;
     AVPacket pkt;
     q = queue_create();
     pkt.data = "SOMETHING";

     int i;
     for (i = 0; i < 100; i++) {
          pkt.pts = i;
          queue_push(q, &pkt);
     }

     for (i = 0; i < 100; i++) {
          queue_pop(q, &pkt);
          fail_unless(pkt.pts == i, NULL);
     }
          
}
END_TEST

void thread_put(void *_q) {
     PacketQueue q = _q;
     AVPacket pkt;
     int i;
     pkt.data = "SOMETHING";
     
     for (i = 0; i < 100; i++) {
          pkt.pts = i;
          queue_push(q, &pkt);
     }
}

START_TEST (test_thread_contention)
{
     AVPacket pkt;
     PacketQueue q;
     pthread_t put;

     q = queue_create();
     pthread_create(&put, NULL, thread_put, q);

     int i = 0;
     while (i < 100) {
          while (queue_pop(q, &pkt) == -1)
               ;
          fail_unless(pkt.pts == i, NULL);
          i++;
     }
}
END_TEST

Suite *queue_suite(void) {
     Suite *s = suite_create("queue");
     TCase *tc_core = tcase_create("core");
     TCase *tc_threads = tcase_create("threads");

     tcase_add_test(tc_core, test_push_pop);
     tcase_add_test(tc_core, test_multiple_push_pop);
     tcase_add_test(tc_threads, test_thread_contention);
     suite_add_tcase(s, tc_core);
     suite_add_tcase(s, tc_threads);

     return s;
}

int main() {
     int nfailed;
     Suite *s = queue_suite();
     SRunner *sr = srunner_create(s);
     srunner_run_all(sr, CK_NORMAL);
     nfailed = srunner_ntests_failed(sr);
     srunner_free(sr);

     return (nfailed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
