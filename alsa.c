#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

snd_pcm_t *init_playback_handle(const char *pcm_device_name, short channels, int rate) {
  int err;

  snd_pcm_t *playback_handle;
  snd_pcm_hw_params_t *hw_params;

  if ((err = snd_pcm_open (&playback_handle, pcm_device_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    fprintf (stderr, "cannot open audio device %s (%s)\n",
       pcm_device_name,
       snd_strerror (err));
    exit (1);
  }
  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
    fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
       snd_strerror (err));
    exit (1);
  }

  if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
       snd_strerror (err));
    exit (1);
  }

  if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf (stderr, "cannot set access type (%s)\n",
       snd_strerror (err));
    exit (1);
  }

  if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
    fprintf (stderr, "cannot set sample format (%s)\n",
       snd_strerror (err));
    exit (1);
  }

  if ((err = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &rate, 0)) < 0) {
    fprintf (stderr, "cannot set sample rate (%s)\n",
       snd_strerror (err));
    exit (1);
  }

  if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, channels)) < 0) {
    fprintf (stderr, "cannot set channel count (%s)\n",
       snd_strerror (err));
    exit (1);
  }

  if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot set parameters (%s)\n",
       snd_strerror (err));
    exit (1);
  }

  snd_pcm_hw_params_free (hw_params);

  if ((err = snd_pcm_prepare (playback_handle)) < 0) {
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
       snd_strerror (err));
    exit (1);
  }

  return playback_handle;
}


/*int main (int argc, char *argv[]) {
  int err;
  short buf[1024];

  snd_pcm_t *playback_handle = init_playback_handle("hw:0,0");

  for (short j = 0; j < 1024; j++) {
    buf[j] = j * j;
  }
  for (int i = 0; i < 4000; i++) {
    if ((err = snd_pcm_writei (playback_handle, buf, 1024)) != 1024) {
      fprintf (stderr, "write to audio interface failed (%s)\n",
         snd_strerror (err));
      exit (1);
    }
  }

  snd_pcm_close (playback_handle);
  exit (0);
}*/
