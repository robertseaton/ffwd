#include <assert.h>

#include "ao.h"
#include "play.h"

int play(AVFrame *frame, int sample_rate, int channels, int ao) {
     int ret = 0;

     switch (ao) {
     case ALSA:
          ret = alsa_play(frame, sample_rate, channels);
          break;
     default:
          assert(1 == 0); /* not implemented */
     }

     return ret;
}
