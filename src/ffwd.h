#ifndef FFWD

#include <libavformat/avformat.h>

void seek(double milliseconds);
void print_metadata(AVFormatContext *format_ctx, char *filename);
void start_playback_threads(AVFormatContext *format_ctx);
void fill_queue_thread(void *_format_ctx);
void set_channels(int c);

#define FFWD
#endif
