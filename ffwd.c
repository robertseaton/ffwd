#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <X11/Xlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ffwd.h"

void metadata(AVFormatContext *format_ctx)
{
     AVDictionaryEntry *tag = NULL;
     
     while ((tag = av_dict_get(format_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
          printf("%s: %s\n", tag->key, tag->value);
}

int get_frame(AVCodecContext *codec_ctx, AVFormatContext *fmt_ctx, int video_stream, AVFrame *frame) {
     AVPacket pkt;
     int got_picture;

     while (av_read_frame(fmt_ctx, &pkt) >= 0) {
          if (pkt.stream_index == video_stream)
               avcodec_decode_video2(codec_ctx, frame, &got_picture, &pkt);
     
          if (got_picture) {
               av_free_packet(&pkt);
               return 1;
          }
     }

     av_free_packet(&pkt);
     return 0;
}


int main(int argc, char *argv[])
{
     AVFormatContext *format_ctx = NULL;
     int video_stream;
     AVCodecContext *codec_ctx;
     AVCodec *codec;
     AVFrame *frame, *rgb_frame;
     
     av_register_all();
     
     if (avformat_open_input(&format_ctx, argv[1], NULL, NULL) != 0) {
          fprintf(stderr, "ERROR: Failed to open file %s\n", argv[1]);
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
     rgb_frame = avcodec_alloc_frame();

     if (get_frame(codec_ctx, format_ctx, video_stream, frame) == 0) {
          fprintf(stderr, "ERROR: Failed to decode video.\n");
          exit(1);
     }

     Display *d;
     Window w;
     GC gc;
     XImage *ximg;
     XWindowAttributes a;
     void *unaligned;

     if ((d = XOpenDisplay(NULL)) == NULL) {
          fprintf(stderr, "ERROR: Failed to open display.\n");
          exit(1);
     }

     w = XCreateWindow(d, DefaultRootWindow(d), 0, 0, 100, 100, 0, CopyFromParent, InputOutput, CopyFromParent, 0, NULL);
     XMapWindow(d, w);
     XFlush(d);

     gc = XCreateGC(d, w, 0, NULL);
     XGetWindowAttributes(d, w, &a);
     ximg = XCreateImage(d, a.visual, 24, ZPixmap, 0, NULL, frame->width, frame->height, 8, 0);
     unaligned = malloc(ximg->bytes_per_line * frame->height + 32);
     ximg->data = unaligned + 16 - ((long)unaligned & 15);
     memset(ximg->data, 0, ximg->bytes_per_line * frame->height);

     struct SwsContext *sws_ctx = NULL;
     uint8_t *dst[4] = {NULL};
     int dstStride[4] = {0};

     dst[0] = ximg->data;
     dstStride[0] = frame->width * ((32 + 7) / 8);
     while (get_frame(codec_ctx, format_ctx, video_stream, frame) != 0) {
          sws_ctx = sws_getCachedContext(sws_ctx,
                                         frame->width, frame->height, frame->format, 
                                         frame->width, frame->height, PIX_FMT_RGB32, 
                                         SWS_BICUBIC, NULL, NULL, 0);

          if (sws_ctx == NULL) {
               fprintf(stderr, "Failed to allocate SwsContext.\n");
               exit(1);
          }
     

          sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, dst, dstStride);

          XPutImage(d, w, gc, ximg, 0, 0, 0, 0, frame->width, frame->height);
          XFlush(d);
     }
}
