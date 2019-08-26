#ifndef AUDIO_CONTEXT_H
#define AUDIO_CONTEXT_H

/**
 * Function called with a buffer to fill with new bytes of audio data.
 */
typedef void(* audio_data_request_cb_t) (void* buffer, size_t num_bytes);

/**
 * Application context for abstracting away the underlying audio code.
 */
typedef struct {
  audio_data_request_cb_t audio_generation_callback;
} app_audio_context;

#endif
