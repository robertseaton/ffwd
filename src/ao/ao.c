#include <assert.h>

#include "ao.h"
#include "ao_internal.h"

void ao_pause(enum audio_output ao) {
     switch (ao) {
     case ALSA:
          alsa_pause();
          break;
     default: 
          assert(1 == 0); /* not implemented */
     }
}

void ao_unpause(enum audio_output ao) {
     switch (ao) {
     case ALSA:
          alsa_unpause();
          break;
     default:
          assert(1 == 0);
     }
}

void ao_flush(enum audio_output ao) {
     switch (ao) {
     case ALSA:
          alsa_flush();
          break;
     default:
          assert(1 == 0);
     }
}

int ao_play(AVFrame *frame, int sample_rate, int channels, enum audio_output ao) {
     int ret = 0;

     switch (ao) {
     case ALSA:
          ret = alsa_play(frame, sample_rate, channels);
          break;
     default:
          assert(1 == 0);
     }

     return ret;
}
