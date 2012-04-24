#ifndef QUEUE

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct _PacketQueue* PacketQueue;

PacketQueue queue_create();
int queue_push(PacketQueue _q, AVPacket *pkt);
int queue_pop(PacketQueue _q, AVPacket *pkt);
int queue_get_size(PacketQueue _q);
pthread_mutex_t *queue_get_mutex(PacketQueue _q);
void queue_flush(PacketQueue _q);
void queue_block_until_needed(PacketQueue audioq, PacketQueue videoq);

#define QUEUE
#endif
