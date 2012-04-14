#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <X11/Xlib.h>

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "vo/vo.h"

struct pkt_queue {
     AVPacketList *first, *last;
     int npkts;
     int size;
     pthread_mutex_t mutex;
} audioq, videoq;

/* information for feeder */
struct trough {
     int astream;
     int vstream;
     AVFormatContext *fmt_ctx;
};

pthread_mutex_t ffmpeg;

void initq(struct pkt_queue *q) {
     q->npkts = 0;
     q->size = 0;
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

     q->last = pkt1;
     q->npkts++;
     q->size += pkt1->pkt.size;

     pthread_mutex_unlock(&q->mutex);

     return 0;
}

int pop(struct pkt_queue *q, AVPacket *pkt) {
     AVPacketList *pkt1;
     
     pthread_mutex_lock(&q->mutex);

     pkt1 = q->first;
     if (pkt1) {
          q->first = pkt1->next;

          if (!q->first)
                    q->last = NULL;

          q->npkts--;
          q->size -= pkt1->pkt.size;
          *pkt = pkt1->pkt;
          av_free(pkt1);
               
     } else
          return -1;

     pthread_mutex_unlock(&q->mutex);
     return 0;
}

void metadata(AVFormatContext *format_ctx) {
     AVDictionaryEntry *tag = NULL;
     
     while ((tag = av_dict_get(format_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
          printf("%s: %s\n", tag->key, tag->value);
}

int get_frame(AVCodecContext *codec_ctx, AVFrame *frame, struct pkt_queue *q) {
     AVPacket pkt;
     int got_frame = 0;

     while (pop(q, &pkt) != -1) {
          if (q == &videoq)
               avcodec_decode_video2(codec_ctx, frame, &got_frame, &pkt);
          else
               avcodec_decode_audio4(codec_ctx, frame, &got_frame, &pkt);

          if (got_frame) {
               av_free_packet(&pkt);
               return 0;
          }
     }

     return -1;
}

int initialize(AVFormatContext *format_ctx, AVCodecContext **codec_ctx, AVCodec **codec, AVFrame **frame, int *stream, int type) {
     *stream = av_find_best_stream(format_ctx, type, -1, -1, NULL, 0);
     if (*stream == AVERROR_STREAM_NOT_FOUND)
          return -1;

     *codec_ctx = format_ctx->streams[*stream]->codec;
     *codec = avcodec_find_decoder((*codec_ctx)->codec_id);

     if (*codec == NULL)
          return -1;

     if ((*codec)->capabilities & CODEC_CAP_TRUNCATED)
          (*codec_ctx)->flags |= CODEC_FLAG_TRUNCATED;

     pthread_mutex_lock(&ffmpeg);
     if (avcodec_open2(*codec_ctx, *codec, NULL) < 0)
          return -1;
     pthread_mutex_unlock(&ffmpeg);
     
     *frame = avcodec_alloc_frame();
     return 0;
}

void audio_loop(void *_format_ctx) {
     AVFormatContext *format_ctx = _format_ctx;
     int stream;
     AVCodecContext *codec_ctx;
     AVCodec *codec;
     AVFrame *frame;
     struct timespec req;

     if (initialize(format_ctx, &codec_ctx, &codec, &frame, &stream, AVMEDIA_TYPE_AUDIO) == -1) {
          fprintf(stderr, "WARNING: Failed to find or decode audio stream. No sound.\n");
          return ;
     }

     if (get_frame(codec_ctx, frame, &audioq) == -1) {
          fprintf(stderr, "WARNING: Failed to decode audio. No sound.\n");
          return ;
     }

     req.tv_sec = 0;
     req.tv_nsec = 1/av_q2d(codec_ctx->time_base) * 1000000; /* miliseconds -> nanoseconds */

     while (get_frame(codec_ctx, frame, &audioq) != -1) {
          assert(frame->pts == AV_NOPTS_VALUE);
          nanosleep(&req, NULL);
     }
}

void video_loop(void *_format_ctx) {
     AVFormatContext *format_ctx = _format_ctx;
     int stream;
     AVCodecContext *codec_ctx;
     AVCodec *codec;
     AVFrame *frame;
     struct timespec req;

     if (initialize(format_ctx, &codec_ctx, &codec, &frame, &stream, AVMEDIA_TYPE_VIDEO) == -1) {
          fprintf(stderr, "ERROR: Failed to initialize video codec.");
          exit(1);
     }

     if (get_frame(codec_ctx, frame, &videoq) == -1) {
          fprintf(stderr, "ERROR: Failed to decode video.\n");
          exit(1);
     }

     req.tv_sec = 0;
     req.tv_nsec = 1/av_q2d(codec_ctx->time_base) * 1000000; /* miliseconds -> nanoseconds */

     while (get_frame(codec_ctx, frame, &videoq) != -1) {
          assert(frame->pts == AV_NOPTS_VALUE);
          nanosleep(&req, NULL);
          if (x11_draw(frame) == -1) {
               fprintf(stderr, "ERROR: Failed to draw frame.\n");
               exit(1);
          }
     }
}

void feeder(void *_t) {
     AVPacket pkt;
     struct trough *t = _t;

     while (av_read_frame(t->fmt_ctx, &pkt) == 0) {
          if (pkt.stream_index == t->vstream)
               push(&videoq, &pkt);
          else if (pkt.stream_index == t->astream)
               push(&audioq, &pkt);
     }
               
}

void usage(char *executable) {
     printf("usage: %s [file]\n", executable);
     exit(0);
}

int main(int argc, char *argv[]) {
     AVFormatContext *format_ctx = NULL;
     pthread_t feedr, audio, video;
     struct trough t;

     if (argc != 2)
          usage(argv[0]);
     
     av_register_all();
     
     if (avformat_open_input(&format_ctx, argv[1], NULL, NULL) != 0) {
          fprintf(stderr, "ERROR: Failed to open file %s.\n", argv[1]);
          exit(1);
     }

     if (avformat_find_stream_info(format_ctx, NULL) < 0) {
          fprintf(stderr, "ERROR: Couldn't find stream information.\n");
          exit(1);
     }

     av_dump_format(format_ctx, 0, argv[1], false);
     metadata(format_ctx);

     initq(&audioq);
     initq(&videoq);
     pthread_mutex_init(&ffmpeg, NULL);

     t.vstream = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
     t.astream = av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
     t.fmt_ctx = format_ctx;

     pthread_create(&feedr, NULL, feeder, &t);
     pthread_create(&video, NULL, video_loop, format_ctx);
     pthread_create(&audio, NULL, audio_loop, format_ctx);

     while (true) {

     }
}
