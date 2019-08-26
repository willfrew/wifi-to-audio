#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pulse/context.h>
#include <pulse/def.h>
#include <pulse/error.h>
#include <pulse/mainloop.h>
#include <pulse/sample.h>
#include <pulse/stream.h>

#include "def.h"

#define APP_NAME "wifi-to-audio"

pa_mainloop_api* mainloop_api;

static const pa_sample_spec sample_spec = {
  format: PA_SAMPLE_U8, // Unsigned 8bit PCM
  rate: 44100,
  channels: 1,
};

void graceful_exit(int code) {
  mainloop_api->quit(mainloop_api, code);
}

void stream_state_callback(pa_stream* stream, void* userdata) {
  switch(pa_stream_get_state(stream)) {
    case PA_STREAM_UNCONNECTED:
    case PA_STREAM_CREATING:
    case PA_STREAM_READY:
      break;

    case PA_STREAM_TERMINATED:
      printf("Stream terminated\n");
      graceful_exit(0);
      break;

    case PA_STREAM_FAILED:
      fprintf(stderr, "Stream failed:\n%s\n", pa_strerror(pa_context_errno((pa_stream_get_context(stream)))));
      graceful_exit(1);
      break;
  }
}

void stream_write_callback(pa_stream* stream, size_t num_bytes, void* userdata) {
  // Ensures that we maintain a continuous sine wave across separate writes.
  static int current_position = 0;

  double tau = 6.28318530; // Radians
  double freq = 440; // Hz
  double rate = 44100; // Bytes/second
  int period = rate / freq; // Bytes
  double conv = tau / period; // Radians / byte

  unsigned char* buffer;

  // Ask the server to initialise our buffer and hand us back a pointer to it
  if ((pa_stream_begin_write(stream, (void**) &buffer, &num_bytes)) != 0) {
    fprintf(stderr, "Failed initialising write buffer:\n%s\n", pa_strerror(pa_context_errno(pa_stream_get_context(stream))));
  }

  ((app_audio_context*) userdata)->audio_generation_callback(buffer, num_bytes);

  for (int i = 0; i < num_bytes; i++) {
    buffer[i] = ((unsigned char) ((sin(current_position * conv) * 127) + 128));
    current_position = (current_position + 1) % period;
  }

  if ((pa_stream_write(stream, buffer, num_bytes, NULL, 0, PA_SEEK_RELATIVE)) != 0) {
    fprintf(stderr, "Failed writing data to stream:\n%s\n", pa_strerror(pa_context_errno(pa_stream_get_context(stream))));
  }
}

void stream_overflow_callback(pa_stream* stream, void* userdata) {
  fprintf(stderr, "Overflow occurred!\n");
}

void stream_underflow_callback(pa_stream* stream, void* userdata) {
  fprintf(stderr, "Underflow occurred!\n");
}

// Callback for connection context state changes
void context_state_callback(pa_context* context, void* userdata) {
  int err;
  pa_stream* stream;

  switch (pa_context_get_state(context)) {
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
      break;

    case PA_CONTEXT_TERMINATED:
      graceful_exit(0);
      break;

    case PA_CONTEXT_READY:
      if (!(stream = pa_stream_new(context, APP_NAME, &sample_spec, NULL))) {
        fprintf(stderr, "Couldn't create audio stream:\n%s\n", pa_strerror(pa_context_errno(context)));
      }

      pa_stream_set_state_callback(stream, stream_state_callback, userdata);
      pa_stream_set_write_callback(stream, stream_write_callback, userdata);
      pa_stream_set_overflow_callback(stream, stream_overflow_callback, userdata);
      pa_stream_set_underflow_callback(stream, stream_underflow_callback, userdata);


      err = pa_stream_connect_playback(
        stream,
        NULL, // Default device
        NULL, // Default buffering attributes
        0, // Default flags
        NULL, // Default volume settings
        NULL // Standalone stream, no synchronisation with other streams
      );

      if (err != 0) {
        fprintf(stderr, "Error connecting playback stream:\n%s\n", pa_strerror(pa_context_errno(context)));
        graceful_exit(1);
      }
      break;

    case PA_CONTEXT_FAILED:
    default:
      fprintf(stderr, "Connection failed:\n%s\n", pa_strerror(pa_context_errno(context)));
      graceful_exit(1);
      break;
  }
}

pa_mainloop* setup_pulseaudio_connection(app_audio_context* app_context) {
  pa_mainloop* mainloop = NULL;
  pa_context* context = NULL;

  if (!pa_sample_spec_valid(&sample_spec)) {
    fprintf(stderr, "Invalid sample specification\n");
    return NULL;
  }

  mainloop = pa_mainloop_new();
  mainloop_api = pa_mainloop_get_api(mainloop);

  if (!(context = pa_context_new(mainloop_api, APP_NAME))) {
    fprintf(stderr, "Couldn't create pulseaudio context\n");
    return NULL;
  }

  pa_context_set_state_callback(context, context_state_callback, app_context);

  if (pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
    fprintf(stderr, "Failed to connect to pulseaudio server:\n%s\n", pa_strerror(pa_context_errno(context)));
    return NULL;
  }

  return mainloop;
}
