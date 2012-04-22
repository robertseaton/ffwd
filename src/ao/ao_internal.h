#ifndef AO_INTERNAL

#include <libavcodec/avcodec.h>

int alsa_play(AVFrame *frame, int sample_rate, int channels, int src_channels);
void alsa_flush();
void alsa_unpause();
void alsa_pause();

#define AO_INTERNAL
#endif
