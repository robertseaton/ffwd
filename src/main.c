#include <libavformat/avformat.h>
#include <getopt.h>
#include <stdbool.h>
#include <pthread.h>
#include <err.h>

#include "ffwd.h"
#include "kbd/kbd.h"

static struct option options[] = {
     {"channels", required_argument, 0, 'c'},
     {0, 0, 0, 0}
};

void usage(char *executable) {
     printf("usage: %s [file]\n", executable);
     printf("\t-c, --channels n\t Force number of channels.\n");
     exit(0);
}

int main(int argc, char *argv[]) {
     AVFormatContext *format_ctx = NULL;
     pthread_t t;
     int c;
     int option_index = 0;

     while (true) {
          c = getopt_long(argc, argv, "c:", options, &option_index);

          if (c == -1)
               break;

          switch (c) {
          case 'c':
               set_channels(atoi(optarg));
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

     print_metadata(format_ctx, argv[optind]);
     pthread_create(&t, NULL, (void *(*)(void *))fill_queue_thread, format_ctx);
     start_playback_threads(format_ctx);
     kbd_event_loop(KBD_X11);
}
