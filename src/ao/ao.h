#ifndef AO

#include <libavcodec/avcodec.h>

enum audio_output {
     ALSA
};

int play(AVFrame *frame, int sample_rate, int channels, int ao);
void alsa_pause();
void alsa_unpause();
void alsa_flush();
#define AO
#endif
