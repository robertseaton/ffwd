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

#include <sys/timeb.h>

#include "queue.h"
#include "ao/ao.h"
#include "vo/vo.h"
#include "kbd/kbd.h"
#include "util.h"

/* information for feeder */
struct trough {
     int astream;
     int vstream;
     AVFormatContext *fmt_ctx;
};

struct pkt_queue audioq, videoq;
pthread_mutex_t ffmpeg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pause_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t is_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t is_paused = PTHREAD_COND_INITIALIZER;
bool paused = false;
double audio_seek_to;
double video_seek_to;
bool seek_backward;
double *start;
AVPacket flush_pkt;

void seek(double seconds) {
     seconds *= 10;
     seconds *= AV_TIME_BASE;
     if (milliseconds < 0)
          seek_backward = true;
     else
          seek_backward = false;
     audio_seek_to = video_seek_to = seconds;
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
          if (pkt.data == flush_pkt.data) {
               avcodec_flush_buffers(codec_ctx);
               while (pop(q, &pkt) == -1)
                    ;
          }

          if (q == &videoq) 
               avcodec_decode_video2(codec_ctx, frame, &got_frame, &pkt);
          else
               avcodec_decode_audio4(codec_ctx, frame, &got_frame, &pkt);

          if (got_frame) {
               av_free_packet(&pkt);
               return 0;
          }
     }
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

int write_to_threshold(AVFrame *frame, AVCodecContext *codec_ctx, int backend) {
     int amount_written = 0;
     int start_threshold;

     start_threshold = play(frame, codec_ctx->sample_rate, codec_ctx->channels, ALSA);

     while (amount_written < start_threshold * 4) {
          if (get_frame(codec_ctx, frame, &audioq) == -1) 
               return -1;

          if (play(frame, codec_ctx->sample_rate, codec_ctx->channels, backend) == -1)
               return -1;

          amount_written += frame->linesize[0];
     }

     return 0;
}

void audio_loop(void *_format_ctx) {
     AVFormatContext *format_ctx = _format_ctx;
     int stream;
     AVCodecContext *codec_ctx;
     AVCodec *codec;
     AVFrame *frame;
     struct timespec req;
     double actual, play_at, paused_at, audio_start;
     double pause_delay = 0;
     bool reset = false;

     if (initialize(format_ctx, &codec_ctx, &codec, &frame, &stream, AVMEDIA_TYPE_AUDIO) == -1) {
          fprintf(stderr, "WARNING: Failed to find or decode audio stream. No sound.\n");
          return ;
     }

     while (get_frame(codec_ctx, frame, &audioq) == -1) 
          ;
     
     if (write_to_threshold(frame, codec_ctx, ALSA) == -1) {
          fprintf(stderr, "ERROR: Failed to write past audio start threshold.\n");
          return ;
     }

     req.tv_sec = 0;
     *start = milliseconds_since_epoch();
     audio_start = *start + frame->pkt_pts;

     while (get_frame(codec_ctx, frame, &audioq) != -1) {
          pthread_mutex_lock(&pause_mutex);
          if (paused == true) {
               paused_at = milliseconds_since_epoch();
               pthread_cond_wait(&is_paused, &pause_mutex);
               pause_delay += milliseconds_since_epoch() - paused_at;
          }
          pthread_mutex_unlock(&pause_mutex);

          actual = milliseconds_since_epoch() - pause_delay;

          if (reset == true) {
               int start_frame = frame->pkt_pts;
               printf("pkt pts %d\n", frame->pkt_pts);
               if (write_to_threshold(frame, codec_ctx, ALSA) == -1) {
                    fprintf(stderr, "ERROR: Failed to write past audio start threshold.\n");
                    return ;
               }
               actual = milliseconds_since_epoch() - pause_delay;
               *start = actual - start_frame;
               audio_start = *start;
               reset = false;
               continue;
          }

          play_at = frame->pkt_pts + audio_start;
          req.tv_nsec = (play_at - actual) * 1000000; /* miliseconds -> nanoseconds */

          nanosleep(&req, NULL);

          if (play(frame, codec_ctx->sample_rate, codec_ctx->channels, ALSA) == -1) {
               fprintf(stderr, "ERROR: Failed to decode audio.\n");
               return ;
          }

          if (audio_seek_to != 0) {
                flush(&audioq);
                push(&audioq, &flush_pkt);
                alsa_flush();
                audio_seek_to = 0;
                reset = true;
          }
     }

     printf("audioq %d\n", audioq.npkts);
     printf("audio finished\n");
}

void video_loop(void *_format_ctx) {
     AVFormatContext *format_ctx = _format_ctx;
     int stream;
     AVCodecContext *codec_ctx;
     AVCodec *codec;
     AVFrame *frame;
     struct timespec req;
     double actual, display_at, paused_at;
     double pause_delay = 0;
     bool reset = false;

     if (initialize(format_ctx, &codec_ctx, &codec, &frame, &stream, AVMEDIA_TYPE_VIDEO) == -1) {
          fprintf(stderr, "ERROR: Failed to initialize video codec.");
          exit(1);
     }

     while (get_frame(codec_ctx, frame, &videoq) == -1)
          ;

     while (start == 0)
          ;

     req.tv_sec = 0;

     while (get_frame(codec_ctx, frame, &videoq) != -1) {
          pthread_mutex_lock(&pause_mutex);
          if (paused == true) {
               paused_at = milliseconds_since_epoch();
               pthread_cond_wait(&is_paused, &pause_mutex);
               pause_delay += milliseconds_since_epoch() - paused_at;
          }
          pthread_mutex_unlock(&pause_mutex);

          actual = milliseconds_since_epoch() - pause_delay;

          display_at = frame->pkt_pts + *start;
          display_at = (display_at - actual) * 1000000; /* miliseconds -> nanoseconds */
          req.tv_nsec = display_at;

          if (display_at < 0)
               continue; /* frame drop */

          nanosleep(&req, NULL);
          if (draw(frame, X11) == -1) {
               fprintf(stderr, "ERROR: Failed to draw frame.\n");
               exit(1);
          }

          if (video_seek_to != 0) {
               video_seek_to += (frame->pkt_pts * AV_TIME_BASE) / 1000;
               if (seek_backward == true) {
                    if (av_seek_frame(format_ctx, -1, video_seek_to, AVSEEK_FLAG_BACKWARD) < 0)
                         fprintf(stderr, "ERROR: Seek failed.\n");
               } else {
                    if (av_seek_frame(format_ctx, -1, video_seek_to, 0) < 0)
                         fprintf(stderr, "ERROR: Seek failed.\n");
               }

               flush(&videoq);
               push(&videoq, &flush_pkt);
               video_seek_to = 0;
          }
     }
     exit(0);
}

void feeder(void *_t) {
     AVPacket pkt;
     struct trough *t = _t;

     while (av_read_frame(t->fmt_ctx, &pkt) == 0) { 
          if (pkt.stream_index == t->vstream)
               push(&videoq, &pkt);
          else if (pkt.stream_index == t->astream)
               push(&audioq, &pkt);

          pthread_mutex_lock(&audioq.mutex);
          pthread_mutex_lock(&videoq.mutex);
          if (videoq.npkts > MAX_QUEUED_PACKETS && audioq.npkts > MAX_QUEUED_PACKETS) {
               pthread_mutex_unlock(&videoq.mutex);
               pthread_cond_wait(&is_full, &audioq.mutex);
          } else
               pthread_mutex_unlock(&videoq.mutex);
          pthread_mutex_unlock(&audioq.mutex);
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

     double _start = 0;
     start = &_start;

     t.vstream = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
     t.astream = av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
     t.fmt_ctx = format_ctx;

     av_init_packet(&flush_pkt);
     flush_pkt.data = "FLUSH";

     pthread_create(&feedr, NULL, feeder, &t);
     pthread_create(&video, NULL, video_loop, format_ctx);
     pthread_create(&audio, NULL, audio_loop, format_ctx);

     kbd_event_loop(KBD_X11);
}
