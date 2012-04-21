#include <pthread.h>
#include <stdlib.h>

#include "queue.h"

extern pthread_cond_t is_full;

typedef struct pkt_queue {
     AVPacketList *first, *last;
     int npkts;
     pthread_mutex_t mutex;
} PacketQueueStruct;

PacketQueue queue_create() {
     PacketQueueStruct *q = malloc(sizeof(PacketQueueStruct));
     q->first = NULL;
     q->last = NULL;
     q->npkts = 0;
     pthread_mutex_init(&q->mutex, NULL);
}

int queue_push(PacketQueue _q, AVPacket *pkt) {
     AVPacketList *pkt1;
     PacketQueueStruct *q = _q;

     if (strcmp(pkt->data, "FLUSH") != 0 && av_dup_packet(pkt) < 0)
          return -1;

     pkt1 = malloc(sizeof(AVPacketList));
     pkt1->pkt = *pkt;
     pkt1->next = NULL;

     pthread_mutex_lock(&q->mutex);

     if (q->last == NULL)
          q->first = pkt1;
     else
          q->last->next = pkt1;

     q->npkts++;
     q->last = pkt1;

     pthread_mutex_unlock(&q->mutex);

     return 0;
}

int queue_pop(PacketQueue _q, AVPacket *pkt) {
     PacketQueueStruct *q = _q;
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

          if (q->npkts < MIN_QUEUED_PACKETS)
               pthread_cond_signal(&is_full);
     } else
          ret = -1;

     pthread_mutex_unlock(&q->mutex);
     return ret;
}

int queue_get_size(PacketQueue _q) {
     PacketQueueStruct *q = _q;
     
     return q->npkts;
}

pthread_mutex_t *queue_get_mutex(PacketQueue _q) {
     PacketQueueStruct *q = _q;
     
     return &q->mutex;
}

void queue_flush(PacketQueue _q) {
     PacketQueueStruct *q = _q;
     AVPacketList *pkt, *pkt1;

     pthread_mutex_lock(&q->mutex);
     for (pkt = q->first; pkt != NULL; pkt = pkt1) {
          pkt1 = pkt->next;
          av_free_packet(&pkt->pkt);
          av_freep(&pkt);
     }

     q->last = NULL;
     q->first = NULL;
     q->npkts = 0;
     pthread_mutex_unlock(&q->mutex);
}
