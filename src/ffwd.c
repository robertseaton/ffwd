#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/audioconvert.h>

#include <X11/Xlib.h>

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <time.h>
#include <err.h>

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
     int sstream;
     AVFormatContext *fmt_ctx;
};

PacketQueue audioq, videoq, subtitleq;
pthread_mutex_t ffmpeg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pause_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t is_paused = PTHREAD_COND_INITIALIZER;
bool paused = false;
double audio_seek_to;
double video_seek_to;
double subtitle_seek_to;
bool seek_backward;
double start = 0;
AVPacket flush_pkt;

int channels = 0;

static struct option options[] = {
     {"channels", required_argument, 0, 'c'},
     {0, 0, 0, 0}
};

void seek(double seconds) {
     seconds *= 10;
     seconds *= AV_TIME_BASE;
     if (seconds < 0)
          seek_backward = true;
     else
          seek_backward = false;
     subtitle_seek_to = audio_seek_to = video_seek_to = seconds;
}

void metadata(AVFormatContext *format_ctx) {
     AVDictionaryEntry *tag = NULL;
     
     while ((tag = av_dict_get(format_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
          printf("%s: %s\n", tag->key, tag->value);
}

int get_frame(AVCodecContext *codec_ctx, AVFrame *frame, PacketQueue q) {
     AVPacket pkt;
     int got_frame = 0;

     while (queue_pop(q, &pkt) != -1) {
          if (pkt.data == flush_pkt.data) {
               avcodec_flush_buffers(codec_ctx);
               while (queue_pop(q, &pkt) == -1)
                    ;
          }

          if (q == videoq) 
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

int initialize(AVFormatContext *format_ctx, AVCodecContext **codec_ctx, AVCodec **codec, AVFrame **frame, int type) {
     unsigned int stream;

     stream = av_find_best_stream(format_ctx, type, -1, -1, NULL, 0);
     if (stream == AVERROR_STREAM_NOT_FOUND)
          return -1;

     *codec_ctx = format_ctx->streams[stream]->codec;
     if (codec != NULL) {
          if (channels != 0)
               (*codec_ctx)->request_channels = channels;

          *codec = avcodec_find_decoder((*codec_ctx)->codec_id);

          if (*codec == NULL)
               return -1;

          if ((*codec)->capabilities & CODEC_CAP_TRUNCATED)
               (*codec_ctx)->flags |= CODEC_FLAG_TRUNCATED;

          pthread_mutex_lock(&ffmpeg);
          if (avcodec_open2(*codec_ctx, *codec, NULL) < 0)
               return -1;
          pthread_mutex_unlock(&ffmpeg);
     }

     if (frame != NULL)
          *frame = avcodec_alloc_frame();

     return 0;
}

int write_to_threshold(AVFrame *frame, AVCodecContext *codec_ctx, int backend) {
     int amount_written = 0;
     int start_threshold;

     start_threshold = alsa_get_threshold();

     while (amount_written < start_threshold) {
          if (get_frame(codec_ctx, frame, audioq) == -1) 
               return -1;

          if (ao_play(frame, backend) == -1)
               return -1;

          amount_written += frame->linesize[0];
     }

     return 0;
}

double wait_if_paused(bool paused) {
     double pause_delay, paused_at;
     pause_delay = 0;

     pthread_mutex_lock(&pause_mutex);
     if (paused == true) {
          paused_at = milliseconds_since_epoch();
          pthread_cond_wait(&is_paused, &pause_mutex);
          pause_delay += milliseconds_since_epoch() - paused_at;
     }
     pthread_mutex_unlock(&pause_mutex);

     return pause_delay;
}

double milliseconds_to_nanoseconds(double milliseconds) {
     return milliseconds * 1000000;
}

void sleep_until_pts(double start, double pts, double pause_delay) {
     double actual, play_at, sleep_for;
     struct timespec t;

     actual = milliseconds_since_epoch() - pause_delay;
     play_at = pts + start;
     sleep_for = play_at - actual;

     t.tv_sec = 0;
     t.tv_nsec = milliseconds_to_nanoseconds(sleep_for);
     nanosleep(&t, NULL);
}

int audio_codec_reconfigure(AVCodecContext *codec_ctx, AVCodec *codec) {
     codec_ctx->request_channels = alsa_get_supported_channels();
     codec_ctx->request_sample_fmt = alsa_get_supported_av_fmt();
     
     codec = avcodec_find_decoder(codec_ctx->codec_id);

     if (codec == NULL)
          return -1;

     if (codec->capabilities & CODEC_CAP_TRUNCATED)
          codec_ctx->flags |= CODEC_FLAG_TRUNCATED;

     pthread_mutex_lock(&ffmpeg);
     if (avcodec_open2(codec_ctx, codec, NULL) < 0)
          return -1;
     pthread_mutex_unlock(&ffmpeg);

     if (codec_ctx->channels != alsa_get_supported_channels())
          errx(1, "ALSA: Failed to reconfigure to a supported number of channels.");
     else if (codec_ctx->sample_rate != alsa_get_supported_sample_rate())
          errx(1, "ALSA: Failed to reconfigure to a supported sample rate.");
     else if (codec_ctx->sample_fmt != alsa_get_supported_av_fmt())
          errx(1, "ALSA: Failed to reconfigure to a supported format.");

     return 0;
}

void audio_loop(void *_format_ctx) {
     AVFormatContext *format_ctx = _format_ctx;
     AVCodecContext *codec_ctx;
     AVCodec *codec;
     AVFrame *frame;
     double actual, audio_start;
     double pause_delay = 0;
     bool reset = false;

     if (initialize(format_ctx, &codec_ctx, &codec, &frame, AVMEDIA_TYPE_AUDIO) == -1) {
          warnx("Failed to find or decode audio stream. No sound.\n");
          return ;
     }

     alsa_initialize(codec_ctx->sample_rate, codec_ctx->channels, codec_ctx->sample_fmt);
     if (audio_codec_reconfigure(codec_ctx, codec) == -1)
          errx(1, "Failed to reconfigure audio codec to supported parameters.");

     while (get_frame(codec_ctx, frame, audioq) == -1) 
          ;

     if (write_to_threshold(frame, codec_ctx, ALSA) == -1) 
          errx(1, "Failed to write past audio start threshold.\n");


     start = milliseconds_since_epoch();
     audio_start = start + frame->pkt_pts;

     while (get_frame(codec_ctx, frame, audioq) != -1) {

          pause_delay = wait_if_paused(paused);
          actual = milliseconds_since_epoch() - pause_delay;

          if (reset == true) {
               int start_frame = frame->pkt_pts;
               if (write_to_threshold(frame, codec_ctx, ALSA) == -1)
                    errx(1, "Failed to write past audio start threshold.");
               actual = milliseconds_since_epoch() - pause_delay;
               start = actual - start_frame;
               audio_start = start;
               reset = false;
               continue;
          }

          sleep_until_pts(audio_start, frame->pkt_pts, pause_delay);

          if (ao_play(frame, ALSA) == -1)
               errx(1, "Failed to decode audio.\n");

          if (audio_seek_to != 0) {
                queue_flush(audioq);
                queue_push(audioq, &flush_pkt);
                ao_flush(ALSA);
                audio_seek_to = 0;
                reset = true;
          }
     }
}

void video_loop(void *_format_ctx) {
     AVFormatContext *format_ctx = _format_ctx;
     AVCodecContext *codec_ctx;
     AVCodec *codec;
     AVFrame *frame;
     double pause_delay = 0;

     if (initialize(format_ctx, &codec_ctx, &codec, &frame, AVMEDIA_TYPE_VIDEO) == -1)
          errx(1, "Failed to initialize video codec.");

     while (get_frame(codec_ctx, frame, videoq) == -1)
          ;

     while (start == 0)
          ;

     while (get_frame(codec_ctx, frame, videoq) != -1) {
          pause_delay = wait_if_paused(paused);
          sleep_until_pts(start, frame->pkt_pts, pause_delay);
          
          if (draw(frame, X11) == -1)
               errx(1, "Failed to draw frame.\n");

          if (video_seek_to != 0) {
               video_seek_to += (frame->pkt_pts * AV_TIME_BASE) / 1000;
               if (seek_backward == true) {
                    if (av_seek_frame(format_ctx, -1, video_seek_to, AVSEEK_FLAG_BACKWARD) < 0)
                         warnx("Seek failed.\n");
               } else {
                    if (av_seek_frame(format_ctx, -1, video_seek_to, 0) < 0)
                         warnx("Seek failed.\n");
               }

               queue_flush(videoq);
               queue_push(videoq, &flush_pkt);
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
               queue_push(videoq, &pkt);
          else if (pkt.stream_index == t->astream)
               queue_push(audioq, &pkt);
          else if (pkt.stream_index == t->sstream)
               queue_push(subtitleq, &pkt);

          queue_block_until_needed(audioq, videoq);
     }
               
}

void usage(char *executable) {
     printf("usage: %s [file]\n", executable);
     printf("\t-c, --channels n\t Force number of channels.\n");
     exit(0);
}

int main(int argc, char *argv[]) {
     AVFormatContext *format_ctx = NULL;
     pthread_t feedr, audio, video;
     struct trough t;
     int c;
     int option_index = 0;

     while (true) {
          c = getopt_long(argc, argv, "c:", options, &option_index);

          if (c == -1)
               break;

          switch (c) {
          case 'c':
               channels = atoi(optarg);
               break;
          default:
               usage(argv[0]);
          }
     }

     if (argc == optind)
          usage(argv[0]);

     av_register_all();
     
     if (avformat_open_input(&format_ctx, argv[optind], NULL, NULL) != 0)
          errx(1, "Failed to open file %s.", argv[optind]);

     if (avformat_find_stream_info(format_ctx, NULL) < 0)
          errx(1, "Couldn't find stream information.");

     av_dump_format(format_ctx, 0, argv[1], false);
     metadata(format_ctx);

     audioq = queue_create();
     videoq = queue_create();
     subtitleq = queue_create();

     t.vstream = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
     t.astream = av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
     t.sstream = av_find_best_stream(format_ctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, NULL, 0);
     t.fmt_ctx = format_ctx;

     av_init_packet(&flush_pkt);
     flush_pkt.data = "FLUSH";

     pthread_create(&feedr, NULL, (void *(*)(void *))feeder, &t);

     if (t.vstream != -1)
          pthread_create(&video, NULL, (void *(*)(void *))video_loop, format_ctx);

     if (t.astream != -1)
          pthread_create(&audio, NULL, (void *(*)(void *))audio_loop, format_ctx);

     kbd_event_loop(KBD_X11);
}
