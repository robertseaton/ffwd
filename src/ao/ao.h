#ifndef AO

#include <libavcodec/avcodec.h>
#include <stdbool.h>

enum audio_output {
     ALSA
};

int ao_play(AVFrame *frame, enum audio_output ao);
void ao_pause(enum audio_output ao);
void ao_unpause(enum audio_output ao);
void ao_flush(enum audio_output ao);
int alsa_initialize(int sample_rate, int channels, int fmt);
int alsa_get_supported_sample_rate();
int alsa_get_supported_channels();
int alsa_get_supported_av_fmt();
int alsa_get_threshold();

#define AO
#endif
