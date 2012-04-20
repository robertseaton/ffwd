#include <stdlib.h>
#include <check.h>
#include <pthread.h>
#include "../src/queue.h"

pthread_cond_t is_full = PTHREAD_COND_INITIALIZER;

START_TEST (test_initq)
{
     struct pkt_queue q;
     initq(&q);

     fail_unless(q.npkts == 0, NULL);
     fail_unless(&q.mutex != NULL, NULL);
}
END_TEST

START_TEST (test_first_push)
{
     struct pkt_queue q;
     AVPacket pkt;
     
     initq(&q);
     pkt.pts = 1337;
     pkt.data = "SOMETHING";
     push(&q, &pkt);

     fail_unless(q.first->pkt.pts == pkt.pts);
}
END_TEST

START_TEST (test_multiple_push)
{
     struct pkt_queue q;
     AVPacket pkt;
     initq(&q);
     pkt.data = "SOMETHING";
     int i;
     for (i = 0; i < 100; i++) {
          pkt.pts = i;
          push(&q, &pkt);
     }

     for (i = 0; i < 100; i++) {
          fail_unless(q.first->pkt.pts == i, NULL);
          q.first = q.first->next;
     }
}
END_TEST

START_TEST (test_single_pop)
{
     struct pkt_queue q;
     AVPacket pkt;
     initq(&q);

     fail_unless(pop(&q, &pkt) == -1, NULL);
}
END_TEST

void empty_or_fail(struct pkt_queue *q) {
     fail_unless(q->npkts == 0, NULL);
     fail_unless(q->first == NULL, NULL);
     fail_unless(q->last == NULL, NULL);
}

START_TEST (test_multiple_pop)
{
     struct pkt_queue q;
     AVPacket pkt;
     initq(&q);

     int i;
     for (i = 0; i < 100; i++)
          fail_unless(pop(&q, &pkt) == -1, NULL);

     empty_or_fail(&q);
}
END_TEST

START_TEST (test_push_pop)
{
     struct pkt_queue q;
     AVPacket pkt, pkt1;
     initq(&q);
     pkt.pts = 1337;
     pkt.data = "SOMETHING";
     push(&q, &pkt);
     pop(&q, &pkt1);
     
     fail_unless(pkt1.pts == 1337, NULL);
     empty_or_fail(&q);
}
END_TEST

START_TEST (test_multiple_push_pop)
{
     struct pkt_queue q;
     AVPacket pkt, pkt1;
     initq(&q);
     pkt.data = "SOMETHING";
     int i;
     for (i = 0; i < 100; i++) {
          pkt.pts = i;
          push(&q, &pkt);
     }

     for (i = 0; i < 100; i++) {
          pop(&q, &pkt);
          fail_unless(pkt.pts == i, NULL);
     }

     empty_or_fail(&q);
          
}
END_TEST

Suite *queue_suite(void) {
     Suite *s = suite_create("queue");
     TCase *tc_core = tcase_create("core");

     tcase_add_test(tc_core, test_initq);
     tcase_add_test(tc_core, test_first_push);
     tcase_add_test(tc_core, test_multiple_push);
     tcase_add_test(tc_core, test_single_pop);
     tcase_add_test(tc_core, test_multiple_pop);
     tcase_add_test(tc_core, test_push_pop);
     tcase_add_test(tc_core, test_multiple_push_pop);
     suite_add_tcase(s, tc_core);

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
