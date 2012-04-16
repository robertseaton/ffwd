#include <assert.h>

#include "ao.h"
#include "play.h"

int play(AVFrame *frame, int sample_rate, int channels, int ao) {
     switch (ao) {
     case ALSA:
          if (alsa_play(frame, sample_rate, channels) == -1)
               return -1;
          break;
     default:
          assert(1 == 0); /* not implemented */
     }

     return 0;
}
