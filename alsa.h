#include <alsa/asoundlib.h>

snd_pcm_t *init_playback_handle(const char *pcm_device_name, short channels, int rate);
