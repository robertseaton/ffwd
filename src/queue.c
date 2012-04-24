#include <pthread.h>
#include <stdlib.h>

#include "queue.h"

#define MAX_QUEUED_PACKETS 100
#define MIN_QUEUED_PACKETS 50

pthread_cond_t is_full = PTHREAD_COND_INITIALIZER;

typedef struct _PacketQueue {
     AVPacketList *first, *last;
     int npkts;
     pthread_mutex_t mutex;
} _PacketQueue;

PacketQueue queue_create() {
     _PacketQueue *q;
     
     q = malloc(sizeof(_PacketQueue));
     q->first = NULL;
     q->last = NULL;
     q->npkts = 0;
     pthread_mutex_init(&q->mutex, NULL);

     return q;
}

int queue_push(PacketQueue _q, AVPacket *pkt) {
     _PacketQueue *q = _q;
     AVPacketList *pkt1;

     if (strcmp((const char *)pkt->data, "FLUSH") != 0 && av_dup_packet(pkt) < 0)
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
     _PacketQueue *q = _q;
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
     _PacketQueue *q = _q;
     
     return q->npkts;
}

pthread_mutex_t *queue_get_mutex(PacketQueue _q) {
     _PacketQueue *q = _q;
     
     return &q->mutex;
}

void queue_flush(PacketQueue _q) {
     _PacketQueue *q = _q;
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

void queue_block_until_needed(PacketQueue audioq, PacketQueue videoq) {
     pthread_mutex_t *audioq_mutex;

     audioq_mutex = queue_get_mutex(audioq);
     pthread_mutex_lock(audioq_mutex);
     if (queue_get_size(audioq) > MAX_QUEUED_PACKETS && queue_get_size(videoq) > MAX_QUEUED_PACKETS)
          pthread_cond_wait(&is_full, audioq_mutex);
     pthread_mutex_unlock(audioq_mutex);
}
