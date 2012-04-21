#ifndef QUEUE

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct PacketQueueStruct *PacketQueue;

PacketQueue queue_create();
int queue_push(PacketQueue _q, AVPacket *pkt);
int queue_pop(PacketQueue _q, AVPacket *pkt);
int queue_get_size(PacketQueue _q);
pthread_mutex_t *queue_get_mutex(PacketQueue _q);
void queue_flush(PacketQueue _q);

#define MAX_QUEUED_PACKETS 100
#define MIN_QUEUED_PACKETS 50

#define QUEUE
#endif
