#ifndef AO

#include <libavcodec/avcodec.h>

enum audio_output {
     ALSA
};

int ao_play(AVFrame *frame, int sample_rate, int channels, enum audio_output ao);
void ao_pause(enum audio_output ao);
void ao_unpause(enum audio_output ao);
void ao_flush(enum audio_output ao);
#define AO
#endif
