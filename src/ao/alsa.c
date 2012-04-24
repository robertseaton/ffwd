#include <stdbool.h>
#include <err.h>

#include <alsa/asoundlib.h>
#include <libavcodec/avcodec.h>

int alsa_initialized = false;
snd_pcm_t *pcm_handle;
snd_pcm_uframes_t threshold = -1;
unsigned int supported_channels = 0;
unsigned int supported_fmt = 0;
unsigned int supported_sample_rate = 0;

void alsa_pause() {
     snd_pcm_pause(pcm_handle, 1);
}

void alsa_unpause() {
     snd_pcm_pause(pcm_handle, 0);
}

void alsa_flush() {
     snd_pcm_drop(pcm_handle);
     snd_pcm_prepare(pcm_handle);
}

enum _snd_pcm_format av_format_to_alsa_format(enum AVSampleFormat fmt) {
     switch (fmt) {
     case AV_SAMPLE_FMT_U8:
          return SND_PCM_FORMAT_U8;
     case AV_SAMPLE_FMT_S16:
          return SND_PCM_FORMAT_S16;
     case AV_SAMPLE_FMT_S32:
          return SND_PCM_FORMAT_S32;
     case AV_SAMPLE_FMT_FLT:
          return SND_PCM_FORMAT_FLOAT;
     default:
          errx(1, "ALSA: No corresponding ALSA format to AVFormat.");
     }
}

enum AVSampleFormat alsa_format_to_av_format(enum _snd_pcm_format fmt) {
     switch (fmt) {
     case SND_PCM_FORMAT_U8:
          return AV_SAMPLE_FMT_U8;
     case SND_PCM_FORMAT_S16:
          return AV_SAMPLE_FMT_S16;
     case SND_PCM_FORMAT_S32:
          return AV_SAMPLE_FMT_S32;
     case SND_PCM_FORMAT_FLOAT:
          return AV_SAMPLE_FMT_FLT;
     default:
          errx(1, "ALSA: No corresponding AVFormat to ALSA format.");
     }
}

enum AVSampleFormat av_format_next_format(enum AVSampleFormat fmt) {
     enum AVSampleFormat last_fmt = AV_SAMPLE_FMT_FLT;

     if (fmt == last_fmt)
          return 0;
     else
          return fmt++;
}

int alsa_initialize2(int sample_rate, int channels, int fmt) {
     snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
     snd_pcm_sw_params_t *swparams;
     snd_pcm_hw_params_t *hwparams;
     char *pcm_name;
     unsigned int periods = 16;
     snd_pcm_uframes_t boundary, chunksz;

     supported_sample_rate = sample_rate;
     pcm_name = strdup("default");
     snd_pcm_hw_params_alloca(&hwparams);
     snd_pcm_sw_params_alloca(&swparams);

     if (snd_pcm_open(&pcm_handle, pcm_name, stream, 0) < 0)
          return -1;

     if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0)
          return -1;

     if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
          return -1;

     if (snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &periods, 0) < 0)
          return -1;

     if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &supported_sample_rate, 0) < 0)
          return -1;

     if (snd_pcm_hw_params_set_channels_near(pcm_handle, hwparams, (unsigned int *)&channels) < 0)
          return -1;

     if (snd_pcm_hw_params_get_channels(hwparams, &supported_channels) < 0)
          return -1;

     if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, av_format_to_alsa_format(fmt)) >= 0)
          supported_fmt = av_format_to_alsa_format(fmt);
     else {
          enum AVSampleFormat start_fmt = fmt;
          while ((fmt = av_format_next_format(fmt)) != start_fmt) {
               if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, av_format_to_alsa_format(fmt)) >= 0)
                    supported_fmt = av_format_to_alsa_format(fmt);
          }
     }

     if (snd_pcm_hw_params(pcm_handle, hwparams) < 0)
          return -1;

     if (snd_pcm_hw_params_get_period_size_min(hwparams, &chunksz, NULL) < 0)
          return -1;
     
     if (snd_pcm_sw_params_current(pcm_handle, swparams) < 0)
          return -1;

     if (snd_pcm_sw_params_get_boundary(swparams, &boundary) < 0)
          return -1;

     if (snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, chunksz) < 0)
          return -1;

     if (snd_pcm_sw_params_get_start_threshold(swparams, &threshold) < 0)
          return -1;

     if (snd_pcm_sw_params_set_stop_threshold(pcm_handle, swparams, boundary) < 0)
          return -1;

     if (snd_pcm_sw_params_set_silence_size(pcm_handle, swparams, boundary) < 0)
          return -1;

     if (snd_pcm_sw_params(pcm_handle, swparams) < 0)
         return -1;

     return 0;
}

int alsa_get_supported_sample_rate() {
     return supported_sample_rate;
}

int alsa_get_supported_channels() {
     return supported_channels;
}

int alsa_get_supported_av_fmt() {
     return alsa_format_to_av_format(supported_fmt);
}

int alsa_get_threshold() {
     int bytes_per_frame = 2 * supported_channels;

     return threshold * bytes_per_frame * 3; /* i'm clearly not calculating this correctly */
}

void alsa_initialize(int sample_rate, int channels, int fmt) {
     if (alsa_initialize2(sample_rate, channels, fmt) == -1)
          errx(1, "ALSA: Failed to initialize hwparams.");
}

int alsa_play(AVFrame *frame) {
     int bytes_per_frame = 2 * supported_channels;
     if (snd_pcm_writei(pcm_handle, frame->data[0], frame->linesize[0] / bytes_per_frame) < 0)
          return -1;

     return 0;
}
