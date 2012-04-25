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
#include <time.h>
#include <err.h>

#include <sys/timeb.h>

#include "queue.h"
#include "ao/ao.h"
#include "vo/vo.h"
#include "util.h"


PacketQueue audioq, videoq;
pthread_mutex_t ffmpeg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pause_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t is_paused = PTHREAD_COND_INITIALIZER;
bool paused = false;
bool finished = false;
double audio_seek_to, video_seek_to;
bool seek_backward;
double start = 0;
AVPacket flush_pkt;
int channels = 0;

enum {
     TRUNCATED
};

void set_channels(int c);
void seek(double seconds);
void print_metadata(AVFormatContext *format_ctx, char *filename);
int get_frame(AVCodecContext *codec_ctx, AVFrame *frame, PacketQueue q);
AVCodecContext get_codec_ctx(AVFormatContext *format_ctx, int type);
AVCodec get_codec(AVCodecContext *codec_ctx);
void set_codec_flag(AVCodecContext *codec_ctx, AVCodec *codec, int flag);
void set_requested_channels(AVCodecContext *codec_ctx);
void open_codec(AVCodecContext *codec_ctx, AVCodec *codec);
int write_to_threshold(AVFrame *frame, AVCodecContext *codec_ctx, int backend);
double wait_if_paused(bool paused);
double milliseconds_to_nanoseconds(double milliseconds);
void sleep_until_pts(double start, double pts, double pause_delay);
void audio_codec_reconfigure(AVCodecContext *codec_ctx, AVCodec *codec);
void audio_reset(int type);
void video_seek(AVFormatContext *format_ctx, int pts);
void video_loop(void *_format_ctx);
void initialize_flush_packet();
void initialize_queues();
unsigned int find_stream(AVFormatContext *format_ctx, int type);
void start_playback_threads(AVFormatContext *format_ctx);
void fill_queue_thread(void *_format_ctx);
void audio_loop(void *_format_ctx);
void initialize_thread(AVFormatContext *format_ctx, AVCodecContext *codec_ctx, AVCodec *codec, AVFrame **frame, int type);
void wait_until_start_is_set();
bool stream_exists(AVFormatContext *format_ctx, int type);

void fill_queue_thread(void *_format_ctx) {
     AVFormatContext *format_ctx = _format_ctx;
     int video_stream, audio_stream;

     video_stream = find_stream(format_ctx, AVMEDIA_TYPE_VIDEO);
     audio_stream = find_stream(format_ctx, AVMEDIA_TYPE_AUDIO);

     initialize_queues();

     AVPacket pkt;
     while (av_read_frame(format_ctx, &pkt) == 0) { 
          if (pkt.stream_index == video_stream)
               queue_push(videoq, &pkt);
          else if (pkt.stream_index == audio_stream)
               queue_push(audioq, &pkt);

          queue_block_until_needed(audioq, videoq);
     }

     finished = true;         
}

void start_playback_threads(AVFormatContext *format_ctx) {
     pthread_t audio, video;

     if (stream_exists(format_ctx, AVMEDIA_TYPE_VIDEO))
          pthread_create(&video, NULL, (void *(*)(void *))video_loop, format_ctx);

     if (stream_exists(format_ctx, AVMEDIA_TYPE_AUDIO))
        pthread_create(&audio, NULL, (void *(*)(void *))audio_loop, format_ctx);
}

void audio_loop(void *_format_ctx) {
     AVFormatContext *format_ctx = _format_ctx;
     AVCodecContext codec_ctx;
     AVCodec codec;
     AVFrame *frame;
     double actual, audio_start;
     double pause_delay = 0;
     bool reset = false;

     initialize_thread(format_ctx, &codec_ctx, &codec, &frame, AVMEDIA_TYPE_AUDIO);
     alsa_initialize(codec_ctx.sample_rate, codec_ctx.channels, codec_ctx.sample_fmt);
     audio_codec_reconfigure(&codec_ctx, &codec);

     while (get_frame(&codec_ctx, frame, audioq) == -1) 
          ;

     if (write_to_threshold(frame, &codec_ctx, ALSA) == -1) 
          errx(1, "Failed to write past audio start threshold.\n");


     start = milliseconds_since_epoch();
     audio_start = start + frame->pkt_pts;

     while (get_frame(&codec_ctx, frame, audioq) != -1 || finished == false) {

          pause_delay += wait_if_paused(paused);

          if (reset == true) {
               int start_frame = frame->pkt_pts;
               if (write_to_threshold(frame, &codec_ctx, ALSA) == -1)
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
               audio_reset(ALSA);
               reset = true;
          }
     }
}

void video_loop(void *_format_ctx) {
     AVFormatContext *format_ctx = _format_ctx;
     AVCodecContext codec_ctx;
     AVCodec codec;
     AVFrame *frame;
     double pause_delay = 0;

     initialize_thread(format_ctx, &codec_ctx, &codec, &frame, AVMEDIA_TYPE_VIDEO);

     while (get_frame(&codec_ctx, frame, videoq) == -1)
          ;

     wait_until_start_is_set();

     while (get_frame(&codec_ctx, frame, videoq) != -1 || finished == false) {
          pause_delay = wait_if_paused(paused);
          sleep_until_pts(start, frame->pkt_pts, pause_delay);
          
          if (draw(frame, X11) == -1)
               errx(1, "Failed to draw frame.\n");

          if (video_seek_to != 0) 
               video_seek(format_ctx, frame->pkt_pts);
     }

     exit(0);
}

int get_frame(AVCodecContext *codec_ctx, AVFrame *frame, PacketQueue q) {
     AVPacket pkt;
     int got_frame = 0;

     while (queue_pop(q, &pkt) != -1) {
          if (pkt.data == flush_pkt.data) {
               avcodec_flush_buffers(codec_ctx);
               while (queue_pop(q, &pkt) == -1 && finished == false)
                    ;

               if (finished == true)
                    continue;
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

void set_channels(int c) {
     channels = c;
}

void seek(double seconds) {
     seconds *= 10;
     seconds *= AV_TIME_BASE;
     if (seconds < 0)
          seek_backward = true;
     else
          seek_backward = false;
     audio_seek_to = video_seek_to = seconds;
}

void print_metadata(AVFormatContext *format_ctx, char *filename) {
     av_dump_format(format_ctx, 0, filename, false);

     AVDictionaryEntry *tag = NULL;
     while ((tag = av_dict_get(format_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
          printf("%s: %s\n", tag->key, tag->value);
}


AVCodecContext get_codec_ctx(AVFormatContext *format_ctx, int type) {
     unsigned int stream;
     AVCodecContext codec_ctx;

     stream = find_stream(format_ctx, type);
     codec_ctx = *(format_ctx->streams[stream]->codec);
     set_requested_channels(&codec_ctx);

     return codec_ctx;
}

AVCodec get_codec(AVCodecContext *codec_ctx) {
     AVCodec *codec;
     
     codec = avcodec_find_decoder(codec_ctx->codec_id);
     set_codec_flag(codec_ctx, codec, TRUNCATED);
     open_codec(codec_ctx, codec);

     return *codec;
}

void set_codec_flag(AVCodecContext *codec_ctx, AVCodec *codec, int flag) {
     if (flag == TRUNCATED)
          if (codec->capabilities & CODEC_CAP_TRUNCATED)
               codec_ctx->flags |= CODEC_FLAG_TRUNCATED;
}

void set_requested_channels(AVCodecContext *codec_ctx) {
     if (channels != 0)
          codec_ctx->request_channels = channels;
}

void open_codec(AVCodecContext *codec_ctx, AVCodec *codec) {
     pthread_mutex_lock(&ffmpeg);
     if (avcodec_open2(codec_ctx, codec, NULL) < 0)
          errx(1, "Failed to open codec.");
     pthread_mutex_unlock(&ffmpeg);
}

void initialize_thread(AVFormatContext *format_ctx, AVCodecContext *codec_ctx, AVCodec *codec, AVFrame **frame, int type) {
     *codec_ctx = get_codec_ctx(format_ctx, type);
     *codec = get_codec(codec_ctx);
     *frame = avcodec_alloc_frame();
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

void audio_codec_reconfigure(AVCodecContext *codec_ctx, AVCodec *codec) {
     int supported_channels = alsa_get_supported_channels();
     int supported_sample_fmt = alsa_get_supported_av_fmt();
     int supported_sample_rate = alsa_get_supported_sample_rate();

     codec_ctx->request_channels = supported_channels;
     codec_ctx->request_sample_fmt = supported_sample_fmt;
     
     *codec = get_codec(codec_ctx);

     if (codec_ctx->channels != supported_channels)
          errx(1, "ALSA: Failed to reconfigure to a supported number of channels. Requested %d, got %d.", supported_channels, codec_ctx->channels);
     else if (codec_ctx->sample_rate != supported_sample_rate)
          errx(1, "ALSA: This file has a sample rate of %d khz, but your hardware only supports %d khz.", codec_ctx->sample_rate, supported_sample_rate);
     else if (codec_ctx->sample_fmt != supported_sample_fmt)
          errx(1, "ALSA: Failed to reconfigure audio codec to a supported format.");
}

void audio_reset(int type) {
     queue_flush(audioq);
     queue_push(audioq, &flush_pkt);
     ao_flush(type);
     audio_seek_to = 0;
}


void video_reset() {
     queue_flush(videoq);
     queue_push(videoq, &flush_pkt);
     video_seek_to = 0;
}

void video_seek(AVFormatContext *format_ctx, int pts) {
     video_seek_to += (pts * AV_TIME_BASE) / 1000;

     int err;
     if (seek_backward)
          err = av_seek_frame(format_ctx, -1, video_seek_to, AVSEEK_FLAG_BACKWARD);
     else 
          err = av_seek_frame(format_ctx, -1, video_seek_to, 0);
     
     if (err < 0)
          warnx("Seek failed.\n");

     video_reset();
}

void wait_until_start_is_set() {
     while (start == 0)
          ;
}

void initialize_flush_packet() {
     av_init_packet(&flush_pkt);
     flush_pkt.data = "FLUSH";
}

void initialize_queues() {
     initialize_flush_packet();

     audioq = queue_create();
     videoq = queue_create();
}

unsigned int find_stream(AVFormatContext *format_ctx, int type) {
     return av_find_best_stream(format_ctx, type, -1, -1, NULL, 0);
}

bool stream_exists(AVFormatContext *format_ctx, int type) {
     if (find_stream(format_ctx, type) == AVERROR_STREAM_NOT_FOUND)
          return false;
     else
          return true;
}



