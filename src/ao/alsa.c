#include <stdbool.h>

#include <alsa/asoundlib.h>
#include <libavcodec/avcodec.h>

int alsa_initialized = false;
snd_pcm_t *pcm_handle;

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

int alsa_initialize(int sample_rate, int *supported_sample_rate, int channels, int *supported_channels) {
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

     if ((*supported_sample_rate = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &sample_rate, 0)) < 0)
          return -1;

     if (snd_pcm_hw_params_set_channels_near(pcm_handle, hwparams, &channels) < 0)
          return -1;

     snd_pcm_hw_params_get_channels(hwparams, supported_channels);
     snd_pcm_hw_params_get_rate(hwparams, supported_sample_rate, 0);

     if (snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &periods, 0) < 0)
          return -1;

     if (snd_pcm_hw_params(pcm_handle, hwparams) < 0)
          return -1;

     if (snd_pcm_hw_params_get_period_size_min(hwparams, &chunksz, NULL) < 0)
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

     return chunksz;
}

int alsa_play(AVFrame *frame, int sample_rate, int channels) {
     static int err = 0;

     static int supported_channels;
     static int supported_sample_rate;
     if (!alsa_initialized) 
          err = alsa_initialize(sample_rate, &supported_sample_rate, channels, &supported_channels);
         
     if (err == -1)
          return -1;

     int bytes_per_frame = 2 * supported_channels;
     if (snd_pcm_writei(pcm_handle, frame->data[0], frame->linesize[0] / bytes_per_frame) < 0)
          return -1;


     return err * (bytes_per_frame * 2); // double prevents underrun
}
