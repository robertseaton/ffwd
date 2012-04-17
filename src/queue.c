#include <pthread.h>
#include <stdlib.h>

#include "queue.h"

void initq(struct pkt_queue *q) {
     q->npkts = 0;
     pthread_mutex_init(&q->mutex, NULL);
}

int push(struct pkt_queue *q, AVPacket *pkt) {
     AVPacketList *pkt1;

     if (av_dup_packet(pkt) < 0)
          return -1;

     pkt1 = malloc(sizeof(AVPacketList));
     pkt1->pkt = *pkt;
     pkt1->next = NULL;

     pthread_mutex_lock(&q->mutex);

     if (!q->last)
          q->first = pkt1;
     else
          q->last->next = pkt1;

     q->npkts++;
     q->last = pkt1;

     pthread_mutex_unlock(&q->mutex);

     return 0;
}

int pop(struct pkt_queue *q, AVPacket *pkt) {
     AVPacketList *pkt1;
     int ret = 0;

     pthread_mutex_lock(&q->mutex);
     pkt1 = q->first;
     if (pkt1) {
          q->first = pkt1->next;

          if (!q->first)
                    q->last = NULL;

          *pkt = pkt1->pkt;
          av_free(pkt1);
          q->npkts--;     
     } else
          ret = -1;

     pthread_mutex_unlock(&q->mutex);
     return ret;
}
