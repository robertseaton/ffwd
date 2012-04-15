#include <stdbool.h>

#include <alsa/asoundlib.h>
#include <libavcodec/avcodec.h>

int alsa_initialized = false;
snd_pcm_t *pcm_handle;

int alsa_initialize(int sample_rate, int channels) {
     snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
     snd_pcm_hw_params_t *hwparams;
     snd_pcm_sw_params_t *swparams;
     char *pcm_name;
     int periods = 16;
     snd_pcm_uframes_t boundary, chunksz;

     pcm_name = strdup("default");
     snd_pcm_hw_params_alloca(&hwparams);
     snd_pcm_sw_params_alloca(&swparams);

     if (snd_pcm_open(&pcm_handle, pcm_name, stream, 0) < 0)
          return -1;

     if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0)
          return -1;

     if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
          return -1;

     if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE) < 0)
          return -1;

     if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &sample_rate, 0) < 0)
          return -1;

     if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, channels) < 0)
          return -1;

     if (snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &periods, 0) < 0)
          return -1;

     if (snd_pcm_hw_params(pcm_handle, hwparams) < 0)
          return -1;

     if (snd_pcm_hw_params_get_period_size(hwparams, &chunksz, NULL) < 0)
          return -1;

     /* sw parameters */
     if (snd_pcm_sw_params_current(pcm_handle, swparams) < 0)
          return -1;

     if (snd_pcm_sw_params_get_boundary(swparams, &boundary) < 0)
          return -1;

     if (snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, chunksz) < 0)
          return -1;

     if (snd_pcm_sw_params_set_stop_threshold(pcm_handle, swparams, boundary) < 0)
          return -1;

     if (snd_pcm_sw_params_set_silence_size(pcm_handle, swparams, boundary) < 0)
          return -1;

     if (snd_pcm_sw_params(pcm_handle, swparams) < 0)
         return -1;

     alsa_initialized = true;

     return 0;
}

int alsa_play(AVFrame *frame, int sample_rate, int channels) {
     int err;

     if (!alsa_initialized) 
          err = alsa_initialize(sample_rate, channels);
         
     if (err == -1)
          return -1;

     snd_pcm_writei(pcm_handle, frame->data[0], frame->linesize[0] / 4);

     return 0;
}
