#ifndef QUEUE

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

struct pkt_queue {
     AVPacketList *first, *last;
     int npkts;
     pthread_mutex_t mutex;
};

void initq(struct pkt_queue *q);
int push(struct pkt_queue *q, AVPacket *pkt);
int pop(struct pkt_queue *q, AVPacket *pkt);
void flush(struct pkt_queue *q);

#define MAX_QUEUED_PACKETS 100
#define MIN_QUEUED_PACKETS 50

#define QUEUE
#endif
