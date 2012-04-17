#ifndef PLAY

#include <libavcodec/avcodec.h>

int alsa_play(AVFrame *frame, int sample_rate, int channels);

#define PLAY
#endif
