#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <X11/Xlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "vo/vo.h"

void metadata(AVFormatContext *format_ctx)
{
     AVDictionaryEntry *tag = NULL;
     
     while ((tag = av_dict_get(format_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
          printf("%s: %s\n", tag->key, tag->value);
}

int get_frame(AVCodecContext *codec_ctx, AVFormatContext *fmt_ctx, int video_stream, AVFrame *frame) {
     AVPacket pkt;
     int got_picture = 0;

     while (av_read_frame(fmt_ctx, &pkt) >= 0) {
          if (pkt.stream_index == video_stream) {
               avcodec_decode_video2(codec_ctx, frame, &got_picture, &pkt);
          }
     
          if (got_picture) {
               av_free_packet(&pkt);
               return 1;
          }
     }

     av_free_packet(&pkt);
     return 0;
}

void usage(char *executable) {
     printf("usage: %s [file]\n", executable);
     exit(0);
}

int main(int argc, char *argv[]) {
     AVFormatContext *format_ctx = NULL;
     int video_stream;
     AVCodecContext *codec_ctx;
     AVCodec *codec;
     AVFrame *frame;
     struct timespec req;

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

     video_stream = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
     if (video_stream == AVERROR_STREAM_NOT_FOUND) {
          fprintf(stderr, "ERROR: Failed to find a valid video stream.\n");
          exit(1);
     }

     codec_ctx = format_ctx->streams[video_stream]->codec;
     codec = avcodec_find_decoder(codec_ctx->codec_id);

     if (codec->capabilities & CODEC_CAP_TRUNCATED)
          codec_ctx->flags |= CODEC_FLAG_TRUNCATED;

     if (codec == NULL) {
          fprintf(stderr, "ERROR: Failed to find a valid decoder.\n");
          exit(1);
     }

     if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
          fprintf(stderr, "ERROR: Failed to open video codec.\n");
          exit(1);
     }

     frame = avcodec_alloc_frame();
     if (get_frame(codec_ctx, format_ctx, video_stream, frame) == 0) {
          fprintf(stderr, "ERROR: Failed to decode video.\n");
          exit(1);
     }

     req.tv_sec = 0;
     req.tv_nsec = 1/av_q2d(codec_ctx->time_base) * 1000000; /* in nanoseconds */

     while (true) {
          get_frame(codec_ctx, format_ctx, video_stream, frame);
          if (frame->pts == AV_NOPTS_VALUE)
                    nanosleep(&req, NULL);
          if (x11_draw(frame) == -1) {
               fprintf(stderr, "ERROR: Failed to draw frame.\n");
               exit(1);
          }
     }
}
